#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "person.h"
//필요한 경우 헤더 파일과 함수를 추가할 수 있음

#define MAX_RECORD_COUNT 512

char **heaparray;
int heap_element_count = 0;

void addToHeap(const char *record, char **heaparray);
void readPage(FILE *fp, char *pagebuf, int pagenum);
void writePage(FILE *fp, const char *pagebuf, int pagenum);
void buildHeap(FILE *inputfp, char **heaparray);
void makeSortedFile(FILE *outputfp, char **heaparray);

// 과제 설명서대로 구현하는 방식은 각자 다를 수 있지만 약간의 제약을 둡니다.
// 레코드 파일이 페이지 단위로 저장 관리되기 때문에 사용자 프로그램에서 레코드 파일로부터 데이터를 읽고 쓸 때도
// 페이지 단위를 사용합니다. 따라서 아래의 두 함수가 필요합니다.
// 1. readPage(): 주어진 페이지 번호의 페이지 데이터를 프로그램 상으로 읽어와서 pagebuf에 저장한다
// 2. writePage(): 프로그램 상의 pagebuf의 데이터를 주어진 페이지 번호에 저장한다
// 레코드 파일에서 기존의 레코드를 읽거나 새로운 레코드를 쓸 때나
// 모든 I/O는 위의 두 함수를 먼저 호출해야 합니다. 즉, 페이지 단위로 읽거나 써야 합니다.

//
// 페이지 번호에 해당하는 페이지를 주어진 페이지 버퍼에 읽어서 저장한다. 페이지 버퍼는 반드시 페이지 크기와 일치해야 한다.
//
void readPage(FILE *fp, char *pagebuf, int pagenum)
{
	fseek(fp, pagenum * PAGE_SIZE, SEEK_SET); // read할 페이지 위치로 오프셋 이동
	fread(pagebuf, PAGE_SIZE, 1, fp); // 페이지를 read

	return;
}

//
// 페이지 버퍼의 데이터를 주어진 페이지 번호에 해당하는 위치에 저장한다. 페이지 버퍼는 반드시 페이지 크기와 일치해야 한다.
//
void writePage(FILE *fp, const char *pagebuf, int pagenum)
{
	fseek(fp, pagenum * PAGE_SIZE, SEEK_SET); // write 할 페이지 위치로 오프셋 이동
	fwrite(pagebuf, PAGE_SIZE, 1, fp); // 페이지를 write

	return;
}

//
// 주어진 레코드 파일에서 레코드를 읽어 heap을 만들어 나간다. Heap은 배열을 이용하여 저장되며, 
// heap의 생성은 Chap9에서 제시한 알고리즘을 따른다. 레코드를 읽을 때 페이지 단위를 사용한다는 것에 주의해야 한다.
//
void buildHeap(FILE *inputfp, char **heaparray)
{
	char pagebuf[PAGE_SIZE];
	int i, j;
	int page_count;
	int record_count;
	int added_record_count;
	int record_file_info[2];

	readPage(inputfp, pagebuf, 0); // 헤더 페이지 읽어옴
	memcpy(record_file_info, pagebuf, sizeof(record_file_info)); // 헤더페이지에서 페이지, 레코드 개수 저장된 부분 복사
	page_count = record_file_info[0]; // 페이지 개수 저장
	record_count = record_file_info[1]; // 레코드 개수 저장

	added_record_count = 0;
	for (i = 1; i < page_count; ++i) {
		readPage(inputfp, pagebuf, i); // 페이지 차례대로 읽는다

		for (j = 0; j < PAGE_SIZE / RECORD_SIZE; ++j) { // 페이지에 저장할 수 있는 레코드 개수만큼 반복
			if (added_record_count >= record_count) break; // 추가한 레코드 개수가 총 레코드 개수와 같다면 더이상 확인할 레코드가 남아있지 않으므로 반복 종료
			addToHeap(pagebuf + (j * RECORD_SIZE), heaparray); // 해당 레코드를 heap에 추가
			++added_record_count;
		}
	}
}

void addToHeap(const char *record, char **heaparray){
	Person person1;
	Person person2;
	char record_buffer[RECORD_SIZE];
	char *sn;
	int i;
	int parent_index;

	memcpy(record_buffer, record, RECORD_SIZE); // 원본 record가 수정되지 않도록 복사한 record를 생성해 복사본을 이용한다
	sn = strtok(record_buffer, "#"); // record 맨 앞에 있는 주민번호를 구한다
	strcpy(person1.sn, sn); // 위에서 얻은 주민번호를 따로 저장한다

	heaparray[heap_element_count] = (char *) malloc(RECORD_SIZE); // heap sort에 이용할 배열에 새 record를 저장할 공간을 할당받는다
	memcpy(heaparray[heap_element_count], record, RECORD_SIZE); // 새로 할당된 공간에 record를 새로 추가한다

	for (i = heap_element_count; i > 0; i = parent_index) {
		parent_index = (i - 1) / 2; // 부모 노드의 인덱스를 구한다
		memcpy(record_buffer, heaparray[parent_index], RECORD_SIZE); // 부모 record가 수정되지 않도록 record_buffer에 복사한 뒤 복사본을 이용
		sn = strtok(record_buffer, "#"); // record 맨 앞에 있는 주민번호를 구한다
		strcpy(person2.sn, sn); // 위에서 얻은 주민번호를 따로 저장한다

		if (strcmp(person1.sn, person2.sn) < 0) { // 부모 record의 주민번호 > 추가한 record의 주민번호 이면
			//swap
			memcpy(record_buffer, heaparray[i], RECORD_SIZE);
			memcpy(heaparray[i], heaparray[parent_index], RECORD_SIZE);
			memcpy(heaparray[parent_index], record_buffer, RECORD_SIZE);
		} else {
			break;
		}
	}

	++heap_element_count;
	return;
}

//
// 완성한 heap을 이용하여 주민번호를 기준으로 오름차순으로 레코드를 정렬하여 새로운 레코드 파일에 저장한다.
// Heap을 이용한 정렬은 Chap9에서 제시한 알고리즘을 이용한다.
// 레코드를 순서대로 저장할 때도 페이지 단위를 사용한다.
//
void makeSortedFile(FILE *outputfp, char **heaparray)
{
	char pagebuf[PAGE_SIZE];
	int writed_record_count;
	int i, j;
	int page_count;
	int record_count;
	int record_file_info[2];

	memset(pagebuf, 0xff, PAGE_SIZE);
	record_count = heap_element_count; // 레코드 개수 저장
	page_count = ((record_count + 1) / (PAGE_SIZE / RECORD_SIZE)) + 1; // 페이지 개수 저장
	record_file_info[0] = page_count; // 헤더 페이지에 저장할 데이터 만든다
	record_file_info[1] = record_count;
	memcpy(pagebuf, record_file_info, sizeof(record_file_info));
	writePage(outputfp, pagebuf, 0); // 헤더 페이지 write

	writed_record_count = 0;
	for (i = 1; i < page_count; ++i) {
		memset(pagebuf, 0xff, PAGE_SIZE);

		for (j = 0; j < PAGE_SIZE / RECORD_SIZE; ++j) {
			if (writed_record_count >= heap_element_count) { // 모든 레코드를 기록했다면
				break; // 반복 종료
			}
			memcpy(pagebuf + (j * RECORD_SIZE), heaparray[(i - 1) * (PAGE_SIZE / RECORD_SIZE) + j], RECORD_SIZE); // 페이지 버퍼에 레코드 추가
			++writed_record_count;
		}
		writePage(outputfp, pagebuf, i); // 페이지 write
	}

	return;
}

int main(int argc, char *argv[])
{
	FILE *inputfp;	// 입력 레코드 파일의 파일 포인터
	FILE *outputfp;	// 정렬된 레코드 파일의 파일 포인터
	int i;

	if (argc != 4) { // 프로그램 실행 시 전달된 인자의 개수가 잘못됐다면 사용법 출력 후 종료
		fprintf(stderr, "usage : %s s <input record file name> <output record file name>\n", argv[0]);
		exit(1);
	}

	if (strcmp("s", argv[1]) && strcmp("S", argv[1])) { // 프로그램 실행 시 전달된 옵션이 잘못되었다면 사용법 출력 후 종료
		fprintf(stderr, "usage : %s s <input record file name> <output record file name>\n", argv[0]);
		exit(1);
	}

	if ((inputfp = fopen(argv[2], "r")) == NULL) { // input record file이 open되지 않는다면 종료
		fprintf(stderr, "fopen error for %s\n", argv[2]);
		exit(1);
	}

	if ((outputfp = fopen(argv[3], "w")) == NULL) { // output record file이 open되지 않는다면 종료
		fprintf(stderr, "fopen error for %s\n", argv[3]);
		exit(1);
	}


	heaparray = (char **) malloc(sizeof(char *) * MAX_RECORD_COUNT); // heap sort에 사용할 배열 동적할당
	buildHeap(inputfp, heaparray);
	makeSortedFile(outputfp, heaparray);
	for(i = 0; i < heap_element_count; ++i) { // heap sort에 사용한 배열 해제
		free(heaparray[i]);
	}
	free(heaparray);

	return 1;
}
