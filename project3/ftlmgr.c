#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "flash.h"
// 필요한 경우 헤더파일을 추가한다

#define TEMP_FILE_NAME "tmp20160548"

FILE *flashfp;	// fdevicedriver.c에서 사용

int dd_read(int ppn, char *pagebuf);
int dd_write(int ppn, char *pagebuf);
int dd_erase(int pbn);

void createFlashMemory(char *fileName, int numOfPages);
void readPage(char *fileName, int pageNumber);
void eraseBlock(char *filename, int blockNumber);
void inplaceUpdate(char *fileName, int pageNumber, char *sectorData, char *spareData);
//
// 이 함수는 FTL의 역할 중 일부분을 수행하는데 물리적인 저장장치 flash memory에 Flash device driver를 이용하여 데이터를
// 읽고 쓰거나 블록을 소거하는 일을 한다 (동영상 강의를 참조).
// flash memory에 데이터를 읽고 쓰거나 소거하기 위해서 fdevicedriver.c에서 제공하는 인터페이스를
// 호출하면 된다. 이때 해당되는 인터페이스를 호출할 때 연산의 단위를 정확히 사용해야 한다.
// 읽기와 쓰기는 페이지 단위이며 소거는 블록 단위이다.
// 
// memset(), memcpy() 등의 함수를 이용하면 편리하다. 물론, 다른 방법으로 해결해도 무방하다.
int main(int argc, char *argv[])
{	
	char sectorbuf[SECTOR_SIZE];
	char pagebuf[PAGE_SIZE];
	char *blockbuf;
	char option;
	char *fileName;
	int numOfBlocks;
	int pageNumber;
	int blockNumber;

	option = argv[1][0];
	fileName = argv[2];
	
	switch(option){
		case 'c' :
			if(argc != 4){
				fprintf(stderr, "usage : %s c <flashfile> <#blocks>\n", argv[0]);
				break;
			}
			// flash memory 파일 생성: 위에서 선언한 flashfp를 사용하여 flash 파일을 생성한다. 그 이유는 fdevicedriver.c에서 
			//                 flashfp 파일포인터를 extern으로 선언하여 사용하기 때문이다.
			numOfBlocks = atoi(argv[3]);
			if(numOfBlocks)
				createFlashMemory(fileName, numOfBlocks);
			break;
		case 'w' :
			if(argc != 6){
				fprintf(stderr, "usage : %s w <flashfile> <ppn> <sectordata> <sparedata>\n", argv[0]);
				break;
			}
			// 페이지 쓰기: pagebuf의 섹터와 스페어에 각각 입력된 데이터를 정확히 저장하고 난 후 해당 인터페이스를 호출한다
			pageNumber = atoi(argv[3]);
			inplaceUpdate(fileName, pageNumber, argv[4], argv[5]);
			break;
		case 'r' :
			if(argc != 4){
				fprintf(stderr, "usage : %s r <flashfile> <ppn>\n", argv[0]);
				break;
			}
			// 페이지 읽기: pagebuf를 인자로 사용하여 해당 인터페이스를 호출하여 페이지를 읽어 온 후 여기서 섹터 데이터와
			//                  스페어 데이터를 분리해 낸다
			pageNumber = atoi(argv[3]);
			readPage(fileName, pageNumber);
			break;
		case 'e' :
			if(argc != 4){
				fprintf(stderr, "usage : %s e <flashfile> <ppn>\n", argv[0]);
				break;
			}
			blockNumber = atoi(argv[3]);
			eraseBlock(fileName, blockNumber);
			break;
		default :
			break;
	}

	return 0;
}


void createFlashMemory(char *fileName, int numOfPages) {
	int i;
	char blockbuf[BLOCK_SIZE];
	memset(blockbuf, (char)0xff, BLOCK_SIZE);
	
	if((flashfp = fopen(fileName, "w")) == NULL) {
		fprintf(stderr, "fopen error for %s\n", fileName);
		return;
	}

	for(i = 0; i < numOfPages; ++i){
		if(fwrite(blockbuf, BLOCK_SIZE, 1, flashfp) != 1){
			fprintf(stderr, "fwrite error\n");
			return;
		}
	}

	fclose(flashfp);

	return;
}

void readPage(char *fileName, int pageNumber){
	int i;
	char pagebuf[PAGE_SIZE];

	if((flashfp = fopen(fileName, "r")) == NULL) {
		fprintf(stderr, "fopen error for %s\n", fileName);
		return;
	} 
	
	dd_read(pageNumber, pagebuf);
	for(i = 0; (i < SECTOR_SIZE) && (pagebuf[i] != (char)0xff); ++i)
		printf("%c", pagebuf[i]);
	if(i)
		printf(" ");
	for(i = 0; (i < SPARE_SIZE) && ((pagebuf + SECTOR_SIZE)[i] != (char)0xff); ++i)
		printf("%c", (pagebuf + SECTOR_SIZE)[i]);

	fclose(flashfp);

	return;
}

void eraseBlock(char *fileName, int blockNumber){
	FILE *tmpfp;
	FILE *savefp;
	long numOfBlocks;
	char pagebuf[PAGE_SIZE];
	int i, j;

	if((tmpfp = fopen(fileName, "r")) == NULL) {
		fprintf(stderr, "fopen error for %s\n", fileName);
		return;
	} 

	if((flashfp = fopen(TEMP_FILE_NAME, "w")) == NULL) {
		fprintf(stderr, "fopen error for %s\n", TEMP_FILE_NAME);
		return;
	} 

	if(fseek(tmpfp, 0, SEEK_END) != 0){
		fprintf(stderr, "fseek error\n");
		return;
	}

	if((numOfBlocks = ftell(tmpfp)) < 0) {
		fprintf(stderr, "ftell error\n");
		return;
	}

	numOfBlocks /= BLOCK_SIZE;

	fseek(tmpfp, 0, SEEK_SET);

	for(i = 0; i < numOfBlocks; ++i) {
		if(i == blockNumber) {
			dd_erase(blockNumber);
			continue;
		}

		for(j = 0; j < PAGE_NUM; ++j) {
			savefp = flashfp;
			flashfp = tmpfp;
			dd_read(i * PAGE_NUM + j, pagebuf);
			flashfp = savefp;
			fwrite(pagebuf, PAGE_SIZE, 1, flashfp);
		}
	}

	fclose(flashfp);
	fclose(tmpfp);
	unlink(fileName);
	rename(TEMP_FILE_NAME, fileName);
}

void inplaceUpdate(char *fileName, int pageNumber, char *sectorData, char *spareData){
	FILE *tmpfp;
	FILE *savefp;
	long numOfBlocks;
	char pagebuf[PAGE_SIZE];

	int blockNumber = pageNumber / PAGE_NUM;
	int tmpPageNum;
	char *pagebufptr;
	char blockbuf[BLOCK_SIZE];
	int i, j;

	eraseBlock(fileName, blockNumber);

	if((tmpfp = fopen(fileName, "r")) == NULL) {
		fprintf(stderr, "fopen error for %s\n", fileName);
		return;
	} 

	if((flashfp = fopen(TEMP_FILE_NAME, "w")) == NULL) {
		fprintf(stderr, "fopen error for %s\n", TEMP_FILE_NAME);
		return;
	} 

	if(fseek(tmpfp, 0, SEEK_END) != 0){
		fprintf(stderr, "fseek error\n");
		return;
	}

	if((numOfBlocks = ftell(tmpfp)) < 0) {
		fprintf(stderr, "ftell error\n");
		return;
	}

	numOfBlocks /= BLOCK_SIZE;

	fseek(tmpfp, 0, SEEK_SET);

	savefp = flashfp;
	flashfp = tmpfp;
	for(i = 0; i < 4; ++i){
		tmpPageNum = blockNumber * PAGE_NUM + i;
		pagebufptr = blockbuf + i * PAGE_SIZE;
		if(tmpPageNum == pageNumber){
			memset(pagebufptr, (char)0xff, PAGE_SIZE);
			memcpy(pagebufptr, sectorData, strlen(sectorData));
			memcpy(pagebufptr + SECTOR_SIZE, spareData, strlen(spareData));
			continue;
		}	

		dd_read(tmpPageNum, pagebufptr);
	}
	flashfp = savefp;
	fseek(tmpfp, 0, SEEK_SET);

	for(i = 0; i < numOfBlocks; ++i) {
		if(i == blockNumber) {
			for(j = 0; j < 4; ++j){
				tmpPageNum = i * PAGE_NUM + j;
				pagebufptr = blockbuf + j * PAGE_SIZE;
				dd_write(tmpPageNum, pagebufptr);
			}
			continue;
		}

		for(j = 0; j < PAGE_NUM; ++j) {
			savefp = flashfp;
			flashfp = tmpfp;
			dd_read(i * PAGE_NUM + j, pagebuf);
			flashfp = savefp;
			fwrite(pagebuf, PAGE_SIZE, 1, flashfp);
		}
	}

	fclose(flashfp);
	fclose(tmpfp);
	unlink(fileName);
	rename(TEMP_FILE_NAME, fileName);
}
