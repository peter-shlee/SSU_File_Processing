#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "person.h"
//필요한 경우 헤더 파일과 함수를 추가할 수 있음

#define MAX_RECORD_COUNT 256

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

	readPage(inputfp, pagebuf, 0);
	memcpy(record_file_info, pagebuf, sizeof(record_file_info));
	page_count = record_file_info[0];
	record_count = record_file_info[1];

	added_record_count = 0;
	for (i = 1; i < page_count; ++i) {
		readPage(inputfp, pagebuf, i);

		for (j = 0; j < PAGE_SIZE / RECORD_SIZE; ++j) {
			if (added_record_count >= record_count) break;
			addToHeap(pagebuf + (j * RECORD_SIZE), heaparray);
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

	memcpy(record_buffer, record, RECORD_SIZE);
	sn = strtok(record_buffer, "#");
	strcpy(person1.sn, sn);

	memcpy(heaparray[heap_element_count], record, RECORD_SIZE);

	for (i = heap_element_count; i > 0; i = parent_index) {
		parent_index = (i - 1) / 2;
		memcpy(record_buffer, heaparray[parent_index], RECORD_SIZE);
		sn = strtok(record_buffer, "#");
		strcpy(person2.sn, sn);

		if (strcmp(person1.sn, person2.sn) < 0) {
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
	record_count = heap_element_count;
	page_count = ((record_count + 1) / (PAGE_SIZE / RECORD_SIZE)) + 1;
	record_file_info[0] = page_count;
	record_file_info[1] = record_count;
	memcpy(pagebuf, record_file_info, sizeof(record_file_info));
	writePage(outputfp, pagebuf, 0);

	writed_record_count = 0;
	for (i = 1; i < page_count; ++i) {
		memset(pagebuf, 0xff, PAGE_SIZE);

		for (j = 0; j < PAGE_SIZE / RECORD_SIZE; ++j) {
			if (writed_record_count >= heap_element_count) {
				break;
			}
			memcpy(pagebuf + (j * RECORD_SIZE), heaparray[(i - 1) * (PAGE_SIZE / RECORD_SIZE) + j], RECORD_SIZE);
			++writed_record_count;
		}
		writePage(outputfp, pagebuf, i);
	}

	return;
}

int main(int argc, char *argv[])
{
	FILE *inputfp;	// 입력 레코드 파일의 파일 포인터
	FILE *outputfp;	// 정렬된 레코드 파일의 파일 포인터
	int i;

	heaparray = (char **) malloc(sizeof(char *) * MAX_RECORD_COUNT);
	for(i = 0; i < MAX_RECORD_COUNT; ++i) {
		heaparray[i] = (char *) malloc(RECORD_SIZE);
	}

	if (argc != 4) {
		fprintf(stderr, "usage : %s s <input record file name> <output record file name>\n", argv[0]);
		exit(1);
	}

	if (strcmp("s", argv[1]) && strcmp("S", argv[1])) {
		fprintf(stderr, "usage : %s s <input record file name> <output record file name>\n", argv[0]);
		exit(1);
	}

	if ((inputfp = fopen(argv[2], "r")) == NULL) {
		fprintf(stderr, "fopen error for %s\n", argv[2]);
		exit(1);
	}

	if ((outputfp = fopen(argv[3], "w")) == NULL) {
		fprintf(stderr, "fopen error for %s\n", argv[3]);
		exit(1);
	}

	buildHeap(inputfp, heaparray);

	makeSortedFile(outputfp, heaparray);


	return 1;
}
