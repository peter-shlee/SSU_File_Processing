#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "person.h"
//필요한 경우 헤더 파일과 함수를 추가할 수 있음

// 과제 설명서대로 구현하는 방식은 각자 다를 수 있지만 약간의 제약을 둡니다.
// 레코드 파일이 페이지 단위로 저장 관리되기 때문에 사용자 프로그램에서 레코드 파일로부터 데이터를 읽고 쓸 때도
// 페이지 단위를 사용합니다. 따라서 아래의 두 함수가 필요합니다.
// 1. readPage(): 주어진 페이지 번호의 페이지 데이터를 프로그램 상으로 읽어와서 pagebuf에 저장한다
// 2. writePage(): 프로그램 상의 pagebuf의 데이터를 주어진 페이지 번호에 저장한다
// 레코드 파일에서 기존의 레코드를 읽거나 새로운 레코드를 쓰거나 삭제 레코드를 수정할 때나
// 모든 I/O는 위의 두 함수를 먼저 호출해야 합니다. 즉 페이지 단위로 읽거나 써야 합니다.

//
// 페이지 번호에 해당하는 페이지를 주어진 페이지 버퍼에 읽어서 저장한다. 페이지 버퍼는 반드시 페이지 크기와 일치해야 한다.
//
void readPage(FILE *fp, char *pagebuf, int pagenum)
{
	fseek(fp, pagenum * PAGE_SIZE, SEEK_SET);
	fread(pagebuf, PAGE_SIZE, 1, fp); 
	return;
}

//
// 페이지 버퍼의 데이터를 주어진 페이지 번호에 해당하는 위치에 저장한다. 페이지 버퍼는 반드시 페이지 크기와 일치해야 한다.
//
void writePage(FILE *fp, const char *pagebuf, int pagenum)
{
	fseek(fp, pagenum * PAGE_SIZE, SEEK_SET);
	fwrite(pagebuf, PAGE_SIZE, 1, fp); 
	return;
}

//
// 새로운 레코드를 저장할 때 터미널로부터 입력받은 정보를 Person 구조체에 먼저 저장하고, pack() 함수를 사용하여
// 레코드 파일에 저장할 레코드 형태를 recordbuf에 만든다. 그런 후 이 레코드를 저장할 페이지를 readPage()를 통해 프로그램 상에
// 읽어 온 후 pagebuf에 recordbuf에 저장되어 있는 레코드를 저장한다. 그 다음 writePage() 호출하여 pagebuf를 해당 페이지 번호에
// 저장한다. pack() 함수에서 readPage()와 writePage()를 호출하는 것이 아니라 pack()을 호출하는 측에서 pack() 함수 호출 후
// readPage()와 writePage()를 차례로 호출하여 레코드 쓰기를 완성한다는 의미이다.
// 
void pack(char *recordbuf, const Person *p)
{
	sprintf(recordbuf, "%s#%s#%s#%s#%s#%s#", p->sn, p->name, p->age, p->addr, p->phone, p->email);
	return;
}

// 
// 아래의 unpack() 함수는 recordbuf에 저장되어 있는 레코드를 구조체로 변환할 때 사용한다. 이 함수가 언제 호출되는지는
// 위에서 설명한 pack()의 시나리오를 참조하면 된다.
//
void unpack(const char *recordbuf, Person *p)
{
	char *field;
	char tmpRecordbuf[RECORD_SIZE];
	memcpy(tmpRecordbuf, recordbuf, RECORD_SIZE);

	field = strtok(tmpRecordbuf, "#");
	strcpy(p->sn, field);
	field = strtok(NULL, "#");
	strcpy(p->name, field);
	field = strtok(NULL, "#");
	strcpy(p->age, field);
	field = strtok(NULL, "#");
	strcpy(p->addr, field);
	field = strtok(NULL, "#");
	strcpy(p->phone, field);
	field = strtok(NULL, "#");
	strcpy(p->email, field);

	return;
}

//
// 새로운 레코드를 저장하는 기능을 수행하며, 터미널로부터 입력받은 필드값을 구조체에 저장한 후 아래의 insert() 함수를 호출한다.
//
void insert(FILE *fp, const Person *p)
{
	char pagebuf[PAGE_SIZE];
	char recordbuf[RECORD_SIZE];
	int headerbuf[PAGE_SIZE / sizeof(int)];
	int recordsPerPage = PAGE_SIZE / RECORD_SIZE;
	long fsize;
	memset(pagebuf, 0xffff, PAGE_SIZE);
	memset(headerbuf, 0xffff, PAGE_SIZE);
	memset(recordbuf, 0xffff, RECORD_SIZE);

	if (fseek(fp, 0, SEEK_END) < 0) { // 파일의 오프셋을 파일의 맨 끝으로 이동
		fprintf(stderr, "fseek error, %s\n", strerror(errno));
		return;
	}
	if ((fsize = ftell(fp)) == 0) { // 오프셋 위치를 이용해 파일 크기를 구함, 파일의 크기가 0이라면 (첫번째 레코드 삽입이라면)
		// 헤더 페이지를 생성한다
		headerbuf[0] = 2;
		headerbuf[1] = 0;
		headerbuf[2] = -1;
		headerbuf[3] = -1;

		writePage(fp, (char *)headerbuf, 0);
		writePage(fp, pagebuf, 1);
	}

	pack(recordbuf, p);
	readPage(fp, (char *)headerbuf, 0); // 헤더 페이지를 읽어온다
	if (headerbuf[2] != -1) {  // 삭제된 레코드가 존재한다면
		int nextDelRec[2];
		readPage(fp, pagebuf, headerbuf[2]);
		memcpy(nextDelRec, pagebuf + (headerbuf[3] * RECORD_SIZE) + 1, 2 * sizeof(int));
		memcpy(pagebuf + (headerbuf[3] * RECORD_SIZE), recordbuf, RECORD_SIZE);
		writePage(fp, pagebuf, headerbuf[2]);

		memcpy(headerbuf + 2, nextDelRec, 2 * sizeof(int));
		writePage(fp, (char *)headerbuf, 0);
		
	} else { // 삭제된 레코드가 존재하지 않는다면
		if (recordsPerPage * (headerbuf[0] - 1) == headerbuf[1]) {  // 새로운 페이지를 생성해 레코드를 삽입해야 하는 경우
			memcpy(pagebuf, recordbuf, RECORD_SIZE);
			writePage(fp, pagebuf, headerbuf[0]);
			++headerbuf[0];
			++headerbuf[1];
			writePage(fp, (char *)headerbuf, 0);

		} else { // 기존의 페이지에 새 레코드를 삽입할 공간이 남아있는 경우
			int targetRecordNum = (headerbuf[1] % recordsPerPage);
			readPage(fp, pagebuf, headerbuf[0] - 1);
			memcpy(pagebuf + (targetRecordNum * RECORD_SIZE), recordbuf, RECORD_SIZE);
			writePage(fp, pagebuf, headerbuf[0] - 1);
			++headerbuf[1];
			writePage(fp, (char *)headerbuf, 0);
		}
	}

	return;
}

//
// 주민번호와 일치하는 레코드를 찾아서 삭제하는 기능을 수행한다.
//
void delete(FILE *fp, const char *sn)
{
	char pagebuf[PAGE_SIZE];
	char recordbuf[RECORD_SIZE];
	int headerbuf[PAGE_SIZE/sizeof(int)];
	int recordsPerPage = PAGE_SIZE / RECORD_SIZE;
	int i, j;
	Person tmpPerson;

	readPage(fp, (char *)headerbuf, 0);
	for(i = 1; i < headerbuf[0]; ++i) {
		readPage(fp, pagebuf, i);
		for(j = 0; j < recordsPerPage; ++j) {
			memcpy(recordbuf, pagebuf + (j * RECORD_SIZE), RECORD_SIZE);
			if (recordbuf[0] == '*') continue; // 삭제된 레코드라면 continue
			unpack(recordbuf, &tmpPerson);
			if (!strcmp(tmpPerson.sn, sn)) { // 해당 레코드와 삭제할 레코드의 주민번호가 일치한다면 (삭제할 레코드라면)
				int nextDeleteRecord[2];
				nextDeleteRecord[0] = headerbuf[2];
				nextDeleteRecord[1] = headerbuf[3];
				*(pagebuf + (j * RECORD_SIZE)) = '*';
				memcpy(pagebuf + (j * RECORD_SIZE) + 1, nextDeleteRecord, sizeof(2 * sizeof(int)));
				headerbuf[2] = i;
				headerbuf[3] = j;
				
				writePage(fp, pagebuf, i);
				writePage(fp, (char *)headerbuf, 0);

				return;
			}
		}
	}

	return;
}

int main(int argc, char *argv[])
{
	FILE *fp;  // 레코드 파일의 파일 포인터

	if (argc < 3) {
		return 0;
	}

	if ((fp = fopen(argv[2], "r+")) == NULL) {
		fp = fopen(argv[2], "w+"); // 파일 존재하지 않는다면 새로 생성
	}

	if (!strcmp(argv[1], "i")) {
		char recordbuf[RECORD_SIZE];
		if (argc != 9) {
			fclose(fp);
			return 0;
		}

		Person p;
		strcpy(p.sn, argv[3]);
		strcpy(p.name, argv[4]);
		strcpy(p.age, argv[5]);
		strcpy(p.addr, argv[6]);
		strcpy(p.phone, argv[7]);
		strcpy(p.email, argv[8]);

		insert(fp, &p);

	} else if (!strcmp(argv[1], "d")) {
		if (argc != 4) {
			fclose(fp);
			return 0;
		}

		delete(fp, argv[3]);

	} else {
		return 0;
	}

	fclose(fp);
	return 1;
}
