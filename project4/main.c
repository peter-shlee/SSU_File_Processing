#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "sectormap.h"
void ftl_open();
void ftl_print();
void ftl_read(int lsn, char *sectorbuf);
void ftl_write(int lsn, char *sectorbuf);
void createFlashMemory(char *fileName, int numOfBlocks);

extern FILE *flashfp;
int main(){
	char sectorData[512];


	unlink("20160548_flash_file");
	fclose(fopen("20160548_flash_file", "w"));
	createFlashMemory("20160548_flash_file", BLOCKS_PER_DEVICE);

	ftl_open();
	ftl_print();

	memset(sectorData, 0, 512);
	sprintf(sectorData, "0");
	ftl_write(0, sectorData);

	ftl_print();

	memset(sectorData, 0, 512);
	sprintf(sectorData, "1");
	ftl_write(1, sectorData);

	ftl_print();

	memset(sectorData, 0, 512);
	sprintf(sectorData, "2");
	ftl_write(2, sectorData);

	ftl_print();

	memset(sectorData, 0, 512);
	sprintf(sectorData, "3");
	ftl_write(3, sectorData);

	ftl_print();

	memset(sectorData, 0, 512);
	sprintf(sectorData, "new 3");
	ftl_write(3, sectorData);

	ftl_print();

	memset(sectorData, 0, 512);
	sprintf(sectorData, "4");
	ftl_write(4, sectorData);

	ftl_print();

	memset(sectorData, 0, 512);
	sprintf(sectorData, "5");
	ftl_write(5, sectorData);

	ftl_print();

	memset(sectorData, 0, 512);
	sprintf(sectorData, "6");
	ftl_write(6, sectorData);

	ftl_print();

	memset(sectorData, 0, 512);
	sprintf(sectorData, "7");
	ftl_write(7, sectorData);

	ftl_print();

	memset(sectorData, 0, 512);
	sprintf(sectorData, "new 0");
	ftl_write(0, sectorData);

	ftl_print();

	memset(sectorData, 0, 512);
	sprintf(sectorData, "new 4");
	ftl_write(4, sectorData);

	ftl_print();
	
	memset(sectorData, 0, 512);
	sprintf(sectorData, "new 2");
	ftl_write(2, sectorData);

	ftl_print();
	memset(sectorData, 0, 512);
	ftl_read(0, sectorData);
	printf("0: %s\n", sectorData);
	memset(sectorData, 0, 512);
	ftl_read(1, sectorData);
	printf("1: %s\n", sectorData);
	memset(sectorData, 0, 512);
	ftl_read(2, sectorData);
	printf("2: %s\n", sectorData);
	memset(sectorData, 0, 512);
	ftl_read(3, sectorData);
	printf("3: %s\n", sectorData);
	memset(sectorData, 0, 512);
	ftl_read(4, sectorData);
	printf("4: %s\n", sectorData);
	memset(sectorData, 0, 512);
	ftl_read(5, sectorData);
	printf("5: %s\n", sectorData);
	memset(sectorData, 0, 512);
	ftl_read(6, sectorData);
	printf("6: %s\n", sectorData);
	memset(sectorData, 0, 512);
	ftl_read(7, sectorData);
	sectorData[511] = 0;
	printf("7: %s\n", sectorData);

	return 0;
}

void createFlashMemory(char *fileName, int numOfBlocks) {
	int i;
	char blockbuf[BLOCK_SIZE];
	memset(blockbuf, (char)0xff, BLOCK_SIZE);
	
	if((flashfp = fopen(fileName, "r+")) == NULL) {
		fprintf(stderr, "fopen error for %s\n", fileName);
		return;
	}

	for(i = 0; i < numOfBlocks; ++i){
		if(fwrite(blockbuf, BLOCK_SIZE, 1, flashfp) != 1){
			fprintf(stderr, "fwrite error\n");
			return;
		}
	}

	//fclose(flashfp);

	return;
}
