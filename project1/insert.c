#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#define READ_WRITE_BYTES 100

long parseLong(const char *numString);

int main(int argc, char *argv[])
{
	char *buffer; // 파일 입출력에 사용할 버퍼
	char *entry; // 읽어들인 데이터를 버퍼에 쓸 위치를 저장해 놓을 변수
	int fd; // 파일 디스크립터 변수
	int length; // 입출력된 길이를 저장할 변수
	int dataLength; // insert할 데이터의 길이
	int fileSize; // 파일의 크기
	int i; // 반복문에 사용할 인덱스
	int offset; // 파일의 오프셋
	int insertedFileLength; // insert 후의 파일의 크기


	if(argc != 4){ // 프로그램에 전달된 인자의 수가 잘못됐다면 프로그램 종료
		fprintf(stderr, "usage : %s <file> <offset> <data>\n", argv[0]);
		exit(1);
	}

	if((fd = open(argv[1], O_RDONLY)) < 0){ // 첫번째 인자로 전달된 file 읽기 전용 모드로 open
		fprintf(stderr, "open error for %s\n", argv[1]);
		exit(1);
	}

	if((fileSize = lseek(fd, 0, SEEK_END)) < 0){ // open한 파일의 크기 저장
		fprintf(stderr, "lseek error\n");
		exit(1);
	}

	offset = parseLong(argv[2]); // 인자로 전달된 offset long형으로 변환하여 저장
	dataLength = strlen(argv[3]); // 인자로 전달된 insert할 데이터의 길이 저장
	// insert 후의 데이터 길이 계산하여 저장
	if(offset > fileSize)
		insertedFileLength = offset + dataLength;
	else
		insertedFileLength = fileSize + dataLength;

	buffer = (char *)calloc(insertedFileLength, sizeof(char)); // insert 후의 데이터 길이로 버퍼 메모리 할당
	if(buffer == NULL){ // 메모리 할당 실패했다면 프로그램 종료
		fprintf(stderr, "calloc error\n");
		exit(1);
	}

	lseek(fd, 0, SEEK_SET); // 파일의 맨 앞으로 오프셋 변경
	entry = buffer;
	while((length = read(fd, entry, READ_WRITE_BYTES)) > 0) // 100바이트씩 읽어들인다
		entry += length; // 읽은 데이터를 저장할 다음 위치로 이동
	close(fd); // 읽기 종료 후 파일 close
	
	if((fd = open(argv[1], O_WRONLY | O_CREAT | O_TRUNC)) < 0){ // insert한 결과를 출력하기 위해 파일의 데이터를 전부 지우고 다시 open
		fprintf(stderr, "open error for %s\n", argv[1]);
		exit(1);
	}

	for(i = fileSize - 1; i >= offset; --i){ // 새로운 데이터를 삽입하기 위해 삽입할 위치에 있는 기존의 데이터들을 뒤로 옮김
		buffer[i + dataLength] = buffer[i];
	}	
	for(i = 0; i < dataLength; ++i){ // 새로운 데이터 삽입
		buffer[offset + i] = argv[3][i];
	}

	entry = buffer;
	while(1) { // 삽입 완료된 데이터를 100바이트씩 파일에 출력
		if(entry + READ_WRITE_BYTES >= buffer + insertedFileLength){ 
			write(fd, entry, (buffer + insertedFileLength) - entry);
			break;
		}
		length = write(fd, entry, READ_WRITE_BYTES); 
		entry += length;
	}

	close(fd);
	free(buffer); // 할당 했던 메모리 free

	exit(0);	
}

long parseLong(const char *numString){
	int length = strlen(numString);
	int i;
	long number = 0;

	for(i = 0; i < length; ++i){
		if(numString[i] < '0' || numString[i] > '9') {
			fprintf(stderr, "wrong argument\n");
			exit(1);
		}

		number *= 10;
		number += numString[i] - '0';
	}

	return number;
}
