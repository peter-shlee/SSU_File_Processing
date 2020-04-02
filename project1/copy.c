#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#define BUFFER_SIZE 100 // 100 byte씩 입출력

int main(int argc, char *argv[])
{
	int fd1, fd2; // 파일 디스크립터를 저장할 변수
	char buffer[BUFFER_SIZE]; // 파일 입출력에 사용할 버퍼
	int length; // 입출력 된 byte 수를 저장할 변수

	if(argc != 3){ // 프로그램 실행 시 사용자가 전달한 인자가 잘못됐으면 종료
		fprintf(stderr, "usage : %s <source> <dest>\n", argv[0]);
		exit(1);
	}

	if((fd1 = open(argv[1], O_RDONLY)) < 0) { // 첫번째 인자로 받은 파일 오픈
		fprintf(stderr, "open error for %s\n", argv[1]);
		exit(1);
	}

	if((fd2 = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0666)) < 0) { // 두번째 인자로 받은 파일 오픈
		fprintf(stderr, "open error for %s\n", argv[2]);
		exit(1);
	}

	while((length = read(fd1, buffer, BUFFER_SIZE)) > 0) // 100 byte씩 읽고, 쓴다
		write(fd2, buffer, length);

	close(fd1);
	close(fd2);
	exit(0);
}
