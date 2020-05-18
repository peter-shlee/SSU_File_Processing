#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "person.h"

void readPage(FILE *fp, char *pagebuf, int pagenum)
{
	fseek(fp, pagenum * PAGE_SIZE, SEEK_SET);
	fread(pagebuf, PAGE_SIZE, 1, fp); 
	return;
}

void writePage(FILE *fp, const char *pagebuf, int pagenum)
{
	fseek(fp, pagenum * PAGE_SIZE, SEEK_SET);
	fwrite(pagebuf, PAGE_SIZE, 1, fp); 
	return;
}

void pack(char *recordbuf, const Person *p)
{
	sprintf(recordbuf, "%s#%s#%s#%s#%s#%s#", p->sn, p->name, p->age, p->addr, p->phone, p->email);
	return;
}

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

void insert(FILE *fp, const Person *p)
{
	char pagebuf[PAGE_SIZE];
	char recordbuf[RECORD_SIZE];
	int headerbuf[PAGE_SIZE / sizeof(int)];
	int recordsPerPage = PAGE_SIZE / RECORD_SIZE;
	long fsize;
	if (fseek(fp, 0, SEEK_END) < 0) {
		fprintf(stderr, "fseek error, %s\n", strerror(errno));
		return;
	}
	if ((fsize = ftell(fp)) == 0) { // ¿¿¿ ¿¿¿ ¿¿ ¿
		// ¿¿ ¿¿¿ ¿¿
		headerbuf[0] = 2;
		headerbuf[1] = 0;
		headerbuf[2] = -1;
		headerbuf[3] = -1;

		writePage(fp, (char *)headerbuf, 0);
		writePage(fp, pagebuf, 1);
	}

	pack(recordbuf, p);
	readPage(fp, (char *)headerbuf, 0);
	if (headerbuf[2] != -1) { 
		int nextDelRec[2];
		readPage(fp, pagebuf, headerbuf[2]);
		memcpy(nextDelRec, pagebuf + (headerbuf[3] * RECORD_SIZE) + 1, 2 * sizeof(int));
		memcpy(pagebuf + (headerbuf[3] * RECORD_SIZE), recordbuf, RECORD_SIZE);
		writePage(fp, pagebuf, headerbuf[2]);

		memcpy(headerbuf + 2, nextDelRec, 2 * sizeof(int));
		writePage(fp, (char *)headerbuf, 0);
		
	} else { 
		if (recordsPerPage * (headerbuf[0] - 1) == headerbuf[1]) { // ¿¿¿ ¿¿¿¿ ¿¿ ¿¿¿ ¿¿ ¿¿ ¿¿¿¿ ¿¿¿¿ ¿¿ ¿¿
			memcpy(pagebuf, recordbuf, RECORD_SIZE);
			writePage(fp, pagebuf, headerbuf[0]);
			++headerbuf[0];
			++headerbuf[1];
			writePage(fp, (char *)headerbuf, 0);

		} else { // ¿¿¿ ¿¿¿¿ ¿¿ ¿¿¿ ¿¿ ¿¿
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
			if (recordbuf[0] == '*') continue;
			unpack(recordbuf, &tmpPerson);
			if (!strcmp(tmpPerson.sn, sn)) {
				// ¿¿
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
	FILE *fp;

	if (argc < 3) {
		return 0;
	}

	if ((fp = fopen(argv[2], "r+")) == NULL) {
		fp = fopen(argv[2], "w+");
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
