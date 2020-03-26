#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#define BUFFER_SIZE 100

int main(int argc, char *argv[])
{
	int fd1, fd2;
	char buffer[BUFFER_SIZE];
	int length;

	if(argc != 3){
		fprintf(stderr, "usage : %s <source> <dest>\n", argv[0]);
		exit(1);
	}

	if((fd1 = open(argv[1], O_RDONLY)) < 0) {
		fprintf(stderr, "open error for %s\n", argv[1]);
		exit(1);
	}

	if((fd2 = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0666)) < 0) {
		fprintf(stderr, "open error for %s\n", argv[2]);
		exit(1);
	}

	while((length = read(fd1, buffer, BUFFER_SIZE)) > 0)
		write(fd2, buffer, length);

	close(fd1);
	close(fd2);
	exit(0);
}
