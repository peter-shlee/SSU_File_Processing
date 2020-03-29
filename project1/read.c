#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#define BUFFER_LENGTH 101 // 100 byte씩 읽고 쓴다. 나머지 1비트는 널문자를 위한것

long parseLong(const char *numString); // 문자열로 된 숫자를 long형으로 변환하는 함수

int main(int argc, char *argv[])
{
	char buffer[BUFFER_LENGTH]; // 파일 입출력에 사용할 버퍼
	int fd; // 파일 디스크립터 변수
	int length; // 입출력된 길이를 저장할 변수
	int numOfReadBytes; // 사용자에게 보여줄 데이터의 길이 저장하는 변수 
	off_t offset; // 사용자에게 보여줄 데이터의 시작 위치 저장하는 변수

	if(argc != 4) { // 전달받은 인자의 개수가 잘못되었다면 프로그램 종료
		fprintf(stderr,"usage : %s <file> <offset> <bytes>\n", argv[0]);
		exit(1);
	}

	if((fd = open(argv[1], O_RDONLY)) < 0) { // 첫번째 인자로 전달받은 파일 오픈
		fprintf(stderr, "open error for %s\n", argv[1]);
		exit(1);
	}

	offset = parseLong(argv[2]); // 인자로 전달받은 오프셋 long형으로 변환 해 저장
	if(lseek(fd, offset, SEEK_SET) < 0){ // offset 위치로 이동
		fprintf(stderr, "lseek error\n");
		exit(1);
	}

	numOfReadBytes = parseLong(argv[3]); // 읽고 출력할 데이터의 수 저장
	while((length = read(fd, buffer, BUFFER_LENGTH - 1)) > 0 && numOfReadBytes > 0) { // 파일에서 100바이트씩 읽어서 사용자에게 보여줌
		buffer[length] = 0;
		if(numOfReadBytes < length){
			buffer[numOfReadBytes] = 0;
		}
		printf("%s", buffer);
		numOfReadBytes -= length;
	}
	printf("\n");

	close(fd);
	exit(0);
}

// 문자열로 되어있는 숫자를 long형으로 변환하여 리턴하는 함수
long parseLong(const char *numString){
	int length = strlen(numString);
	int i;
	long number = 0;

	for(i = 0; i < length; ++i){
		if(numString[i] < '0' || numString[i] > '9') { // 숫자가 아니라면 프로그램 종료
			fprintf(stderr, "wrong argument\n");
			exit(1);
		}

		number *= 10;
		number += numString[i] - '0';
	}

	return number;
}
