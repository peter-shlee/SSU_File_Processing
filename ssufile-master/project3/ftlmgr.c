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

void createFlashMemory(char *fileName, int numOfBlocks);
void readPage(char *fileName, int pageNumber);
void eraseBlock(char *filename, int blockNumber);
void inplaceUpdate(char *fileName, int pageNumber, char *sectorData, char *spareData);
int writeBlock(char *fileName, int blockNumber, char *blockbuf);
int writePage(char *fileName, long numOfBlock, int pageNumber, char *sectorData, char *spareData);
int isEmptyPage(char *fileName, int pageNumber);
int isEmptyBlock(char *fileName, int blockNumber);
int findEmptyBlock(char *fileName, int numOfBlocks);
void makeNewBlockData(char *fileName, char *blockbuf, int pageNumber, char *sectorData, char *spareData);
void loadBlockData(char *fileName, char *blockbuf, int blockNumber);
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


void createFlashMemory(char *fileName, int numOfBlocks) { // 새로운 flashfile을 생성하는 함수
	int i;
	char blockbuf[BLOCK_SIZE];
	memset(blockbuf, (char)0xff, BLOCK_SIZE);
	
	if((flashfp = fopen(fileName, "w")) == NULL) {
		fprintf(stderr, "fopen error for %s\n", fileName);
		return;
	}

	for(i = 0; i < numOfBlocks; ++i){ // 지정된 블럭 개수만큼
		if(fwrite(blockbuf, BLOCK_SIZE, 1, flashfp) != 1){ // 파일에 1개 블럭 출력
			fprintf(stderr, "fwrite error\n");
			return;
		}
	}

	fclose(flashfp);

	return;
}

void readPage(char *fileName, int pageNumber){ // 페이지의 데이터를 읽어오는 함수
	int i;
	char pagebuf[PAGE_SIZE];

	if((flashfp = fopen(fileName, "r")) == NULL) {
		fprintf(stderr, "fopen error for %s\n", fileName);
		return;
	} 
	
	dd_read(pageNumber, pagebuf); // 페이지의 데이터를 읽어 pagebuf에 저장
	for(i = 0; (i < SECTOR_SIZE) && (pagebuf[i] != (char)0xff); ++i) // 앞에서부터 순차적으로 0xff(데이터 없음)을 만날 때까지 sector data를 한글자씩 출력한다 
		printf("%c", pagebuf[i]);
	if(i) // sector data와 spare data를 구분하기 위해 공백 출력 (sector data로 출력한 것이 있을 때에만)
		printf(" ");
	for(i = 0; (i < SPARE_SIZE) && ((pagebuf + SECTOR_SIZE)[i] != (char)0xff); ++i) // sector data와 같은 방법으로 spare data 출력
		printf("%c", (pagebuf + SECTOR_SIZE)[i]);

	fclose(flashfp);

	return;
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

		for(j = 0; j < PAGE_NUM; ++j) { // erase 하지 않을 블록이라면 그냥 기존 파일의 내용을 새 파일에 복사한다
			savefp = flashfp;
			flashfp = tmpfp;
			dd_read(i * PAGE_NUM + j, pagebuf);
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

int writePage(char *fileName, long numOfBlocks, int pageNumber, char *sectorData, char *spareData) { // 데이터를 페이지에 쓰는 함수
	FILE *tmpfp;
	FILE *savefp;
	char pagebuf[PAGE_SIZE];
	char *pagebufptr;
	int tmpPageNum;
	int i, j;

	if(!isEmptyPage(fileName, pageNumber)){ // 해당 페이지가 비어있지 않다면 데이터를 write할 수 없으므로 바로 함수 종료
		return 0;
	}

	if((tmpfp = fopen(fileName, "r")) == NULL) { // 기존의 flashfile open
		fprintf(stderr, "fopen error for %s\n", fileName);
		return 0;
	} 

	if((flashfp = fopen(TEMP_FILE_NAME, "w")) == NULL) { // 새로운 flashfile open
		fprintf(stderr, "fopen error for %s\n", TEMP_FILE_NAME);
		return 0;
	} 

	for(i = 0; i < numOfBlocks; ++i) {
		for(j = 0; j < PAGE_NUM; ++j) {
			if(i * PAGE_NUM + j == pageNumber) { // 데이터를 write할 페이지라면
				memset(pagebuf, (char)0xff, PAGE_SIZE);
				memcpy(pagebuf, sectorData, strlen(sectorData));
				memcpy(pagebuf + SECTOR_SIZE, spareData, strlen(spareData));
				dd_write(i * PAGE_NUM + j, pagebuf); // write
				continue;
			}
			savefp = flashfp;
			flashfp = tmpfp;
			dd_read(i * PAGE_NUM + j, pagebuf);
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

int writeBlock(char *fileName, int blockNumber, char *blockbuf){ // 블럭의 모든 페이지를 write하는 함수
	FILE *tmpfp;
	FILE *savefp;
	long numOfBlocks;
	char pagebuf[PAGE_SIZE];
	char *pagebufptr;
	int tmpPageNum;
	int i, j;

	if(!isEmptyBlock(fileName, blockNumber)) return 0; // 해당 블럭이 비어있지 않다면 erase를 한 뒤에 write할 수 있으므로 바로 함수를 종료한다
	
	if((tmpfp = fopen(fileName, "r")) == NULL) { // 기존의 flash memory파일 open
		fprintf(stderr, "fopen error for %s\n", fileName);
		return 0;
	} 

	if((flashfp = fopen(TEMP_FILE_NAME, "w")) == NULL) { // 새로운 flash memory 파일 open
		fprintf(stderr, "fopen error for %s\n", TEMP_FILE_NAME);
		return 0;
	} 

	if(fseek(tmpfp, 0, SEEK_END) != 0){
		fprintf(stderr, "fseek error\n");
		return 0;
	}

	if((numOfBlocks = ftell(tmpfp)) < 0) {
		fprintf(stderr, "ftell error\n");
		return 0;
	}
	numOfBlocks /= BLOCK_SIZE; // flash memory의 블럭의 총 개수를 알아낸다

	fseek(tmpfp, 0, SEEK_SET);

	for(i = 0; i < numOfBlocks; ++i) {
		if(i == blockNumber) { // write할 블럭이라면 새로운 데이터를 쓴다
			for(j = 0; j < 4; ++j){
				tmpPageNum = i * PAGE_NUM + j;
				pagebufptr = blockbuf + j * PAGE_SIZE;
				dd_write(tmpPageNum, pagebufptr);
			}
			continue;
		}

		for(j = 0; j < PAGE_NUM; ++j) { // 새로 write를 할 블럭이 아니라면 기존의 데이터를 그냥 옮긴다
			savefp = flashfp;
			flashfp = tmpfp;
			dd_read(i * PAGE_NUM + j, pagebuf);
			flashfp = savefp;
			fwrite(pagebuf, PAGE_SIZE, 1, flashfp); // 실제 flash memory가 아니라 파일에 대해서 작업을 하고 있기 때문에 내용을 수정하려면 새로운 파일을 만들어 swap해야 합니다
								// 이 과정에서 실제 flash memory라면 아무 작업도 하지 않아도 될 블럭들도 새로운 파일로 복사하게 됩니다
								// 이런 데이터는 fdevicedriver의 함수를 사용하지 않고 fwrite를 사용했습니다.
		}
	}

	fclose(flashfp);
	fclose(tmpfp);
	unlink(fileName); // 기존의 flashfile을 unlink하고
	rename(TEMP_FILE_NAME, fileName); // 새로운 flashfile의 이름을 기존의 flashfile 이름으로 변경한다
	return 1;
}

int isEmptyPage(char *fileName, int pageNumber){ // 해당 페이지가 비어있는지 확인한다
	int j;
	char pagebuf[PAGE_SIZE]; // 페이지를 읽어와 임시로 저장할 버퍼

	if((flashfp = fopen(fileName, "r")) == NULL) {
		fprintf(stderr, "fopen error for %s\n", fileName);
		return 0;
	} 

	dd_read(pageNumber, pagebuf); // 페이지의 데이터를 읽어온다
	for(j = 0; (j < PAGE_SIZE); ++j){ // 페이지의 데이터를 앞에서부터 순차적으로 확인한다
		if(pagebuf[j] != (char)0xff){ // 페이지에 데이터가 들어있다면
			fclose(flashfp);
			return 0; // 0을 리턴한다
		}
	}

	fclose(flashfp);
	return 1; // 페이지에 데이터가 들어있지 않다면 1을 리턴한다
}

int isEmptyBlock(char *fileName, int blockNumber){ // 해당 블럭이 빈 블럭인지 확인하는 함수
	int k, j;
	char pagebuf[PAGE_SIZE]; // 페이지를 읽어와 임시로 저장할 버퍼
	int emptyPageFlag;
	int emptyBlockFlag;

	if((flashfp = fopen(fileName, "r")) == NULL) {
		fprintf(stderr, "fopen error for %s\n", fileName);
		return 0;
	} 

	emptyBlockFlag = 1;
	for(k = 0; k < PAGE_NUM; ++k) { // 블럭의 첫번째 페이지부터 순차적으로 확인한다
		emptyPageFlag = 1;
		dd_read(blockNumber * PAGE_NUM + k, pagebuf); // 페이지의 데이터를 읽어온다
		for(j = 0; (j < PAGE_SIZE); ++j){ // 페이지의 데이터를 앞에서부터 순차적으로 확인한다
			if(pagebuf[j] != (char)0xff){ // 페이지에 데이터가 들어있다면
				emptyPageFlag = 0;
				emptyBlockFlag = 0;
				break; // 반복 종료
			}
		}
		if(!emptyPageFlag) break;
	}

	if(emptyBlockFlag){ // 블럭이 비어있다면
		fclose(flashfp);
		return 1; // 1 리턴
	}

	fclose(flashfp);
	return 0; // 블럭이 비어있지 않으면 0 리턴
}

int findEmptyBlock(char *fileName, int numOfBlocks){ // 비어있는 블럭을 찾아내 그 블럭의 인덱스를 리턴하는 함수
	int i;

	for(i = 0; i < numOfBlocks; ++i) { // flash memory의 앞쪽 블럭에서부터 순차적으로 빈 블럭이 있는지 확인한다
		if(isEmptyBlock(fileName, i)) return i; // 빈 블럭을 발견했다면 그 인덱스를 바로 리턴한다
	}

	return -1; // 빈 블럭을 발견하지 못했다면 -1을 리턴한다
}

void makeNewBlockData(char *fileName, char *blockbuf, int pageNumber, char *sectorData, char *spareData){ // 블럭에 write할 데이터를 만드는 함수
	int i;
	int tmpPageNum;
	int blockNumber = pageNumber / PAGE_NUM;
	char *pagebufptr;

	if((flashfp = fopen(fileName, "r")) == NULL) { // 기존에 block에 저장되어있던 데이터를 읽어오기 위해 파일 open
		fprintf(stderr, "fopen error for %s\n", fileName);
		return;
	} 

	memset(blockbuf, (char)0xff, BLOCK_SIZE); // 새 데이터를 저장할 배열을 초기화

	for(i = 0; i < 4; ++i){
		tmpPageNum = blockNumber * PAGE_NUM + i;
		pagebufptr = blockbuf + i * PAGE_SIZE;
		if(tmpPageNum == pageNumber){ // 새로운 데이터를 기록할 페이지라면 새로운 데이터를 넣는다
			memset(pagebufptr, (char)0xff, PAGE_SIZE);
			memcpy(pagebufptr, sectorData, strlen(sectorData));
			memcpy(pagebufptr + SECTOR_SIZE, spareData, strlen(spareData));
			continue;
		}	

		dd_read(tmpPageNum, pagebufptr); // 기존의 데이터를 그대로 넣을 페이지라면 기존의 데이터를 그대로 넣는다
	}

	fclose(flashfp);
}

void loadBlockData(char *fileName, char *blockbuf, int blockNumber){ // 블럭의 데이터를 메모리로 읽어오는 함수
	int i;
	int tmpPageNum;
	char *pagebufptr;

	if((flashfp = fopen(fileName, "r")) == NULL) { // 데이터를 읽기 위해 파일 오픈
		fprintf(stderr, "fopen error for %s\n", fileName);
		return;
	} 

	memset(blockbuf, (char)0xff, BLOCK_SIZE); // 데이터를 불러올 메모리를 0xff로 초기화한다

	for(i = 0; i < 4; ++i){ // 해당 블럭의 데이터들을 읽어온다
		tmpPageNum = blockNumber * PAGE_NUM + i;
		pagebufptr = blockbuf + i * PAGE_SIZE;
		dd_read(tmpPageNum, pagebufptr);
	}

	fclose(flashfp);
}

void inplaceUpdate(char *fileName, int pageNumber, char *sectorData, char *spareData){ // 인자로 전달된 page에 data를 write하는 함수
	FILE *tmpfp;
	long numOfBlocks;
	int blockNumber = pageNumber / PAGE_NUM;
	int tmpBlockNum;
	char blockbuf[BLOCK_SIZE];

	// flash memory의 block들의 총 개수를 구하기 위해 파일 open
	if((tmpfp = fopen(fileName, "r")) == NULL) {
		fprintf(stderr, "fopen error for %s\n", fileName);
		return;
	} 

	if(fseek(tmpfp, 0, SEEK_END) != 0){ // 파일의 크기를 구하기 위해 오프셋을 파일의 끝으로 이동
		fprintf(stderr, "fseek error\n");
		return;
	}

	if((numOfBlocks = ftell(tmpfp)) < 0) { // 파일의 크기를 구한다
		fprintf(stderr, "ftell error\n");
		return;
	}
	numOfBlocks /= BLOCK_SIZE; // 파일의 크기를 블럭의 크기로 나눠 블럭의 총 개수를 구한다
	fclose(tmpfp); // block들의 개수를 구했으므로 파일 close
	
	if(isEmptyPage(fileName, pageNumber)){ // 비어있는 page라면
		writePage(fileName, numOfBlocks, pageNumber, sectorData, spareData); // 그냥 해당 page에 write
	} else {// write할 page에 데이터가 들어있는 경우
		tmpBlockNum = findEmptyBlock(fileName, numOfBlocks); // inplace update에 사용할 empty블락이 있는지 확인한다
		if(tmpBlockNum == -1){ // empty block이 없다면
			fprintf(stderr, "no empty block in flash memory\n"); // inplace update가 불가능 하므로 종료한다
			return;
		}
		makeNewBlockData(fileName, blockbuf, pageNumber, sectorData, spareData); // inplace update할 블럭에 있는 기존의 데이터와 새로 write할 데이터를 합쳐 블럭에 들어갈 새로운 데이터를 만들어 blockbuf에 저장해 둔다
		writeBlock(fileName, tmpBlockNum, blockbuf); // 일단 데이터를 다른 empty 블럭으로 copy
		eraseBlock(fileName, blockNumber); // block을 erase한다
		loadBlockData(fileName, blockbuf, tmpBlockNum); // empty 블럭에 임시 저장했던 데이터를 불러온다
		writeBlock(fileName, blockNumber, blockbuf); // 위에서 불러온 데이터를 제 위치에 write한다 (recopy)
		eraseBlock(fileName, tmpBlockNum); // 임시로 사용했던 block에 erase를 하여 다시 empty 블럭으로 만든다

	}
}
