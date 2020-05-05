// 주의사항
// 1. sectormap.h에 정의되어 있는 상수 변수를 우선적으로 사용해야 함
// 2. sectormap.h에 정의되어 있지 않을 경우 본인이 이 파일에서 만들어서 사용하면 됨
// 3. 필요한 data structure가 필요하면 이 파일에서 정의해서 쓰기 바람(sectormap.h에 추가하면 안됨)

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include "sectormap.h"
// 필요한 경우 헤더 파일을 추가하시오.

//
// flash memory를 처음 사용할 때 필요한 초기화 작업, 예를 들면 address mapping table에 대한
// 초기화 등의 작업을 수행한다. 따라서, 첫 번째 ftl_write() 또는 ftl_read()가 호출되기 전에
// file system에 의해 반드시 먼저 호출이 되어야 한다.
//

#define FLASH_FILE_NAME "20160548_flash_file"
#define TEMP_FILE_NAME "20160548_temp_file"

FILE *flashfp; // fdevicedriver.c에서 사용하는 파일 포인터
int *addressMappingTable; // sector mapping 방식의 address mapping table
int *garbagePageCnt; // 블럭별 garbage 페이지 개수 배열
int *erasedPageArray; // 내용 삭제되어 바로 write할 수 있는 페이지 배열
int freeBlock; // freeBlock 인덱스

int dd_read(int ppn, char *pagebuf); // fdevicedriver.c의 함수
int dd_write(int ppn, char *pagebuf); // fdevicedriver.c의 함수
int dd_erase(int pbn); // fdevicedriver.c의 함수

void createFlashMemory(char *fileName, int numOfPages); // 플래시 메모리 파일 생성하는 함수
int writePage(char *fileName, long numOfBlocks, int pageNumber, char *sectorData, char *spareData); // page단위로 write하는 함수
void eraseBlock(char *fileName, int blockNumber); // 해당 블럭을 erase하는 함수

void ftl_open()
{
	int i;

	// address mapping table 초기화
    	// address mapping table에서 lbn 수는 DATABLKS_PER_DEVICE 동일
	addressMappingTable = (int *)malloc(DATAPAGES_PER_DEVICE * sizeof(int));
	for(i = 0; i < DATAPAGES_PER_DEVICE; ++i) 
		addressMappingTable[i] = -1;

	// file I/O에 사용할 여러 배열들 생성
	garbagePageCnt = (int *)calloc(BLOCKS_PER_DEVICE ,sizeof(int));
	erasedPageArray = (int *)malloc(BLOCKS_PER_DEVICE * PAGES_PER_BLOCK * sizeof(int));
	for(i = 0; i < BLOCKS_PER_DEVICE * PAGES_PER_BLOCK; ++i)
		erasedPageArray[i] = TRUE;

	createFlashMemory(FLASH_FILE_NAME, BLOCKS_PER_DEVICE * PAGES_PER_BLOCK);
	
	// free block's pbn 초기화
	freeBlock = DATABLKS_PER_DEVICE;


	return;
}

//
// 이 함수를 호출하기 전에 이미 sectorbuf가 가리키는 곳에 512B의 메모리가 할당되어 있어야 한다.
// 즉, 이 함수에서 메모리를 할당받으면 안된다.
//
void ftl_read(int lsn, char *sectorbuf)
{
	int i;
	char pagebuf[PAGE_SIZE];

	if((flashfp = fopen(FLASH_FILE_NAME, "r")) == NULL) {
		fprintf(stderr, "fopen error for %s\n", FLASH_FILE_NAME);
		return;
	} 
	
	if(addressMappingTable[lsn] == -1){
		memset(sectorbuf, 0, SECTOR_SIZE);
	       	return; // 이 경우 sector buf를 어떻게 해야 하는가
	}
	dd_read(addressMappingTable[lsn], pagebuf);
	memcpy(sectorbuf, pagebuf, SECTOR_SIZE);

	fclose(flashfp);

	return;
}


void ftl_write(int lsn, char *sectorbuf)
{
	int psn;
	int i, j;
	SpareData spareData;
	spareData.lpn = lsn;
	spareData.is_invalid = TRUE;

	if(addressMappingTable[lsn] == -1) { // 해당 lsn이 아직 한번도 쓰여진 적이 없는 경우
		int emptyPageAvailableFlag = FALSE;
		for(i = 0; i < PAGES_PER_BLOCK * BLOCKS_PER_DEVICE; ++i) {
			// free 블럭의 페이지인지 확인
			if (i / PAGES_PER_BLOCK == freeBlock) continue;

			if(erasedPageArray[i]) { // 데이터가 저장되어 있지 않은 페이지라면
				emptyPageAvailableFlag = TRUE;
				erasedPageArray[i] = FALSE;
				addressMappingTable[lsn] = i;

				writePage(FLASH_FILE_NAME, BLOCKS_PER_DEVICE, i, sectorbuf, (char *)&spareData);
				break;
			}


		}

		if(!emptyPageAvailableFlag) { // 비어있는 페이지가 없다면
			// free블럭 활용
			int newFreeBlock = 0;
			int maxGarbageCnt = 0;
			int i;
			int nextWritePage;

			++garbagePageCnt[addressMappingTable[lsn] / PAGES_PER_BLOCK];
			addressMappingTable[lsn] = -1;

			// garbage 페이지가 가장 많은 블록을 다음 free블록으로 선정
			for(i = 0; i < BLOCKS_PER_DEVICE; ++i){
				if(garbagePageCnt[i] > maxGarbageCnt) {
					maxGarbageCnt = garbagePageCnt[i];
					newFreeBlock = i;
				}
			}

			nextWritePage = freeBlock * PAGES_PER_BLOCK;

			// 기존 데이터 중 아직 유효한 페이지를 기존의 free 블럭으로 옮김 - 유효한 페이지는 "addressMapping 테이블에서 찾을 수 있는가"로 확인
			for(i = 0; i < PAGES_PER_BLOCK; ++i) {
				int validPageFlag = FALSE;
				for(j = 0; j < DATAPAGES_PER_DEVICE; ++j) {
					if(addressMappingTable[j] == newFreeBlock * PAGES_PER_BLOCK + i) {
						validPageFlag = TRUE;
						break;
					}
				}

				if(validPageFlag) {
					char pagebuf[PAGE_SIZE];

					// 기존 값 불러온 뒤 새로운 페이지에 저장
					if((flashfp = fopen(FLASH_FILE_NAME, "r")) == NULL) {
						fprintf(stderr, "fopen error for %s\n", FLASH_FILE_NAME);
						return;
					} 
					
					dd_read(addressMappingTable[j], pagebuf);

					fclose(flashfp);

					writePage(FLASH_FILE_NAME, BLOCKS_PER_DEVICE, nextWritePage, pagebuf, pagebuf + SECTOR_SIZE);

					// addressMappingTable값 변경
					erasedPageArray[nextWritePage] = FALSE;
					addressMappingTable[j] = nextWritePage++;
				}
				

			}
			// 새로운 데이터 write
			writePage(FLASH_FILE_NAME, BLOCKS_PER_DEVICE, nextWritePage, sectorbuf, (char *)&spareData);
			// addressMappingTable값 변경
			erasedPageArray[nextWritePage] = FALSE;
			addressMappingTable[lsn] = nextWritePage;

			// 기존 블럭 erase
			eraseBlock(FLASH_FILE_NAME, newFreeBlock);
			garbagePageCnt[newFreeBlock] = 0;

			// erase된 페이지들 erasedPageArray 값 1로 변경
			for(i = 0; i < PAGES_PER_BLOCK; ++i){
				erasedPageArray[newFreeBlock * PAGES_PER_BLOCK + i] = TRUE;
			}
			// free블록 값 변경
			freeBlock = newFreeBlock;
		}
	} else { // 해당 lsn이 이미 쓰여있는 경우
		int emptyPageAvailableFlag = FALSE;
		for(i = 0; i < PAGES_PER_BLOCK * BLOCKS_PER_DEVICE; ++i) {
			// free 블럭의 페이지인지 확인
			if (i / PAGES_PER_BLOCK == freeBlock) continue;

			if(erasedPageArray[i]) { // 데이터가 저장되어 있지 않은 페이지라면
				SpareData spareData;
				spareData.lpn = lsn;
				spareData.is_invalid = TRUE;

				emptyPageAvailableFlag = TRUE;
				erasedPageArray[i] = FALSE;
				++garbagePageCnt[addressMappingTable[lsn] / PAGES_PER_BLOCK];
				addressMappingTable[lsn] = i;

				writePage(FLASH_FILE_NAME, BLOCKS_PER_DEVICE, i, sectorbuf, (char *)&spareData);
				break;
			}
		}
		if(!emptyPageAvailableFlag) { // 비어있는 페이지가 없다면
			// free블럭 활용
			int newFreeBlock = 0;
			int maxGarbageCnt = 0;
			int i;
			int nextWritePage;

			++garbagePageCnt[addressMappingTable[lsn] / PAGES_PER_BLOCK];
//			printf("lsn:%d, psn:%d, e:%d\n", lsn, addressMappingTable[lsn], addressMappingTable[lsn] / PAGES_PER_BLOCK);
			addressMappingTable[lsn] = -1;

			// garbage 페이지가 가장 많은 블록을 다음 free블록으로 선정
			for(i = 0; i < BLOCKS_PER_DEVICE; ++i){
//				printf("%d:%d, ", i, garbagePageCnt[i]);
				if(garbagePageCnt[i] > maxGarbageCnt) {
					maxGarbageCnt = garbagePageCnt[i];
					newFreeBlock = i;
				}
			}
//			printf("\n");

//			printf("newFreeBlock : %d\n", newFreeBlock);
			nextWritePage = freeBlock * PAGES_PER_BLOCK;

			// 기존 데이터 중 아직 유효한 페이지를 기존의 free 블럭으로 옮김 - 유효한 페이지는 "addressMapping 테이블에서 찾을 수 있는가"로 확인
			for(i = 0; i < PAGES_PER_BLOCK; ++i) {
				int validPageFlag = FALSE;
				for(j = 0; j < DATAPAGES_PER_DEVICE; ++j) {
					if(addressMappingTable[j] == newFreeBlock * PAGES_PER_BLOCK + i) {
						validPageFlag = TRUE;
						break;
					}
				}

				if(validPageFlag) {
					char pagebuf[PAGE_SIZE];

					// 기존 값 불러온 뒤 새로운 페이지에 저장
					if((flashfp = fopen(FLASH_FILE_NAME, "r")) == NULL) {
						fprintf(stderr, "fopen error for %s\n", FLASH_FILE_NAME);
						return;
					} 
					
					dd_read(addressMappingTable[j], pagebuf);

					fclose(flashfp);

					writePage(FLASH_FILE_NAME, BLOCKS_PER_DEVICE, nextWritePage, pagebuf, pagebuf + SECTOR_SIZE);

					// addressMappingTable값 변경
					erasedPageArray[nextWritePage] = FALSE;
					addressMappingTable[j] = nextWritePage++;
				}
				

			}
			// 새로운 데이터 write
			writePage(FLASH_FILE_NAME, BLOCKS_PER_DEVICE, nextWritePage, sectorbuf, (char *)&spareData);
			// addressMappingTable값 변경
			erasedPageArray[nextWritePage] = FALSE;
			addressMappingTable[lsn] = nextWritePage;

			// 기존 블럭 erase
			eraseBlock(FLASH_FILE_NAME, newFreeBlock);
			garbagePageCnt[newFreeBlock] = 0;

			// erase된 페이지들 erasedPageArray 값 1로 변경
			for(i = 0; i < PAGES_PER_BLOCK; ++i){
				erasedPageArray[newFreeBlock * PAGES_PER_BLOCK + i] = TRUE;
			}
			// free블록 값 변경
			freeBlock = newFreeBlock;
		}


	}

//	for(i = 0; i < BLOCKS_PER_DEVICE * PAGES_PER_BLOCK; ++i) {
//		printf("%d:%d, ", i, erasedPageArray[i]);
//	}
//	printf("\n");

	return;
}

int writePage(char *fileName, long numOfBlocks, int pageNumber, char *sectorData, char *spareData) { // 데이터를 페이지에 쓰는 함수
	FILE *tmpfp;
	FILE *savefp;
	char pagebuf[PAGE_SIZE];
	char *pagebufptr;
	int tmpPageNum;
	int i, j;

	if((tmpfp = fopen(fileName, "r")) == NULL) { // 기존의 flashfile open
		fprintf(stderr, "fopen error for %s\n", fileName);
		return 0;
	} 

	if((flashfp = fopen(TEMP_FILE_NAME, "w")) == NULL) { // 새로운 flashfile open
		fprintf(stderr, "fopen error for %s\n", TEMP_FILE_NAME);
		return 0;
	} 

	for(i = 0; i < numOfBlocks; ++i) {
		for(j = 0; j < PAGES_PER_BLOCK; ++j) {
			if(i * PAGES_PER_BLOCK + j == pageNumber) { // 데이터를 write할 페이지라면
				memset(pagebuf, (char)0xff, PAGE_SIZE);
				memcpy(pagebuf, sectorData, strlen(sectorData));
				memcpy(pagebuf + SECTOR_SIZE, spareData, strlen(spareData));
				dd_write(i * PAGES_PER_BLOCK + j, pagebuf); // write
				continue;
			}
			savefp = flashfp;
			flashfp = tmpfp;
			dd_read(i * PAGES_PER_BLOCK + j, pagebuf);
			flashfp = savefp;
			fwrite(pagebuf, PAGE_SIZE, 1, flashfp); // write하지 않고 가만히 놔둘 페이지는 그냥 내용을 복사한다
		}
	}

	fclose(flashfp);
	fclose(tmpfp);
	unlink(fileName);
	rename(TEMP_FILE_NAME, fileName);
	return 1;
}

void eraseBlock(char *fileName, int blockNumber){ // 블록을 erase 하는 함수
	FILE *tmpfp;
	FILE *savefp;
	long numOfBlocks;
	char pagebuf[PAGE_SIZE];
	int i, j;

	if((tmpfp = fopen(fileName, "r")) == NULL) { // 기존의 flashfile open
		fprintf(stderr, "fopen error for %s\n", fileName);
		return;
	} 

	if((flashfp = fopen(TEMP_FILE_NAME, "w")) == NULL) { // 새로운 flashfile open
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

	numOfBlocks /= BLOCK_SIZE; // flash memory의 블록의 총 개수를 구한다

	fseek(tmpfp, 0, SEEK_SET);

	for(i = 0; i < numOfBlocks; ++i) {
		if(i == blockNumber) { // erase할 블록이라면
			dd_erase(blockNumber);
			continue;
		}

		for(j = 0; j < PAGES_PER_BLOCK; ++j) { // erase 하지 않을 블록이라면 그냥 기존 파일의 내용을 새 파일에 복사한다
			savefp = flashfp;
			flashfp = tmpfp;
			dd_read(i * PAGES_PER_BLOCK + j, pagebuf);
			flashfp = savefp;
			fwrite(pagebuf, PAGE_SIZE, 1, flashfp);	// 실제 flash memory가 아니라 파일에 대해서 작업을 하고 있기 때문에 내용을 수정하려면 새로운 파일을 만들어 swap해야 한다
								// 이 과정에서 실제 flash memory라면 아무 작업도 하지 않아도 될 블럭들도 새로운 파일로 복사해야한다
								// 이런 데이터는 fdevicedriver의 함수를 사용하지 않고 fwrite()를 사용했다
		}
	}

	fclose(flashfp);
	fclose(tmpfp);
	unlink(fileName);
	rename(TEMP_FILE_NAME, fileName);
}

void ftl_print()
{
	int i;

	printf("lpn\tppn\n");
	for(i = 0; i < DATAPAGES_PER_DEVICE; ++i) {
		printf("%d\t%d\n", i, addressMappingTable[i]);
	}
	printf("free block's pbn=%d\n", freeBlock);

	return;
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
