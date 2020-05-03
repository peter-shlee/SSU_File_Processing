// 주의사항
// 1. sectormap.h에 정의되어 있는 상수 변수를 우선적으로 사용해야 함
// 2. sectormap.h에 정의되어 있지 않을 경우 본인이 이 파일에서 만들어서 사용하면 됨
// 3. 필요한 data structure가 필요하면 이 파일에서 정의해서 쓰기 바람(sectormap.h에 추가하면 안됨)

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include "sectormap.h"
// 필요한 경우 헤더 파일을 추가하시오.

//
// flash memory를 처음 사용할 때 필요한 초기화 작업, 예를 들면 address mapping table에 대한
// 초기화 등의 작업을 수행한다. 따라서, 첫 번째 ftl_write() 또는 ftl_read()가 호출되기 전에
// file system에 의해 반드시 먼저 호출이 되어야 한다.
//

#define LSN 0
#define PSN 1
#define FLASH_FILE_NAME "20160548_flash_file"

FILE *flashfp;
int **addressMappingTable;
int freeBlock;

int dd_read(int ppn, char *pagebuf);
int dd_write(int ppn, char *pagebuf);
int dd_erase(int pbn);

void createFlashMemory(char *fileName, int numOfPages);

void ftl_open()
{
	//
	// address mapping table 초기화
	// free block's pbn 초기화
    	// address mapping table에서 lbn 수는 DATABLKS_PER_DEVICE 동일
	int i;


	addressMappingTable = (int **)malloc(DATAPAGES_PER_DEVICE * sizeof(int*));
	for(i = 0; i < DATAPAGES_PER_DEVICE; ++i) 
		addressMappingTable[i] = (int *)malloc(2 * sizeof(int));

	for(i = 0; i < DATAPAGES_PER_DEVICE; ++i) {
		addressMappingTable[i][LSN] = i;
		addressMappingTable[i][PSN] = -1;
	}

	printf("mappingtable c\n");

	createFlashMemory(FLASH_FILE_NAME, BLOCKS_PER_DEVICE * PAGES_PER_BLOCK);
	printf("createfile c\n");
	
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
	
	if(addressMappingTable[lsn][PSN] == -1){
		memset(sectorbuf, 0, SECTOR_SIZE);
	       	return; // 이 경우 sector buf를 어떻게 해야 하는가
	}
	dd_read(addressMappingTable[lsn][PSN], pagebuf);
	memcpy(sectorbuf, pagebuf, SECTOR_SIZE);

	fclose(flashfp);

	return;
}


void ftl_write(int lsn, char *sectorbuf)
{

	return;
}

void ftl_print()
{
	int i;

	printf("lpn\tppn\n");
	for(i = 0; i < DATAPAGES_PER_DEVICE; ++i) {
		printf("%d\t%d\n", addressMappingTable[i][LSN], addressMappingTable[i][PSN]);
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
