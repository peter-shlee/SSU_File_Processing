#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#define BUFFER_SIZE 100 // 100바이트씩 입출력

int main(int argc, char *argv[])
{
	char buffer[BUFFER_SIZE]; // 파일 입출력에 사용할 버퍼
	int fd1, fd2; // 파일 디스크립터 변수들
	int length; // 입출력된 길이 저장할 변수

	if(argc != 3){ // 프로그램에 전달된 인자의 수가 잘못됐다면 프로그램 종료
		fprintf(stderr, "usage : %s <file 1> <file 2>\n", argv[0]);
		exit(1);
	}

	if((fd1 = open(argv[1], O_WRONLY | O_APPEND)) < 0) { // 첫번째 인자로 전달된 파일 쓰기 전용 모드로 open
		fprintf(stderr, "open error for %s\n", argv[1]);
		exit(1);
	}

	if((fd2 = open(argv[2], O_RDONLY)) < 0) { // 두번째 인자로 전달된 파일 읽기 전용 모드로 open
		fprintf(stderr, "open error for %s\n", argv[2]);
		exit(1);
	}

	while((length = read(fd2, buffer, BUFFER_SIZE)) > 0) // fd2에서 100바이트씩 읽어들임 
		write(fd1, buffer, length); // fd1에 읽어 들인 데이터 write

	close(fd1);
	close(fd2);
	exit(0);
}
