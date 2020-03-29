#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#define READ_WRITE_BYTES 100 // 100 byte씩 입출력

long parseLong(const char *numString);

int main(int argc, char *argv[])
{
	char *buffer; // 파일 입출력에 사용할 버퍼
	char *entry; // 읽어들인 데이터를 버퍼에 쓸 위치를 저장해 놓을 변수
	int fd; // 파일 디스크립터 변수
	int length; // 입출력된 길이를 저장할 변수
	int deleteBytes; // delete할 데이터의 길이
	int fileSize; // 파일의 크기
	int deletedFileLength; // delete 후의 파일의 크기
	int i; // 반복문에 사용할 인덱스
	int offset; // 파일의 오프셋

	if(argc != 4){ // 프로그램에 전달된 인자의 수가 잘못됐다면 프로그램 종료
		fprintf(stderr
			, "usage : %s <file> <offset> <delete bytes>\n"
			, argv[0]);
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
	deleteBytes = parseLong(argv[3]); // 인자로 전달된 delete할 길이 저장

	if(offset > fileSize || !deleteBytes) // delete할 것이 없다면 바로 종료
		exit(0);

	// delete 후의 데이터 길이를 계산하여 저장
	if((fileSize - (offset + deleteBytes)) < 0)
		deletedFileLength = offset;
	else
		deletedFileLength = offset + fileSize - (offset + deleteBytes);	

	buffer = (char *)malloc(fileSize); // 파일의 크기만큼 버퍼 메모리 할당
	if(buffer == NULL){ // 메모리 할당 실패했다면 프로그램 종료
		fprintf(stderr, "malloc error\n");
		exit(1);
	}

	lseek(fd, 0, SEEK_SET);
	entry = buffer;
	while((length = read(fd, entry, READ_WRITE_BYTES)) > 0) // 100바이트씩 읽어들인다
		entry += length;
	close(fd); // 읽기 종료 후 파일 close

	for(i = offset; i < deletedFileLength; ++i){ // delete
		buffer[i] = buffer[i + deleteBytes];
	}

	if((fd = open(argv[1], O_WRONLY | O_CREAT | O_TRUNC)) < 0){ // delete 완료한 데이터를 파일에 저장하기 위해 파일을 초기화 하여 다시 open
		fprintf(stderr, "open error for %s\n", argv[1]);
		exit(1);
	}
	entry = buffer;
	while(1) { // delete 완료된 데이터를 100바이트씩 파일에 출력
		if(entry + READ_WRITE_BYTES >= buffer + deletedFileLength){
			write(fd, entry, (buffer + deletedFileLength) - entry);
			break;
		}
		length = write(fd, entry, READ_WRITE_BYTES); 
		entry += length;
	}
	
	close(fd); // 할당 했던 메모리 free
	free(buffer);

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
