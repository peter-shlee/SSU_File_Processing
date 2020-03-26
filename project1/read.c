#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

long parseLong(const char *numString);

int main(int argc, char *argv[])
{
	char c;
	int fd;
	off_t numOfBytes;
	off_t offset;

	if(argc != 4) {
		fprintf(stderr,"usage : %s <file> <offset> <bytes>\n", argv[0]);
		exit(1);
	}

	if((fd = open(argv[1], O_RDONLY)) < 0) {
		fprintf(stderr, "open error for %s\n", argv[1]);
		exit(1);
	}

	offset = parseLong(argv[2]);
	if(lseek(fd, offset, SEEK_SET) < 0){
		fprintf(stderr, "lseek error\n");
		exit(1);
	}

	numOfBytes = parseLong(argv[3]);
	while(read(fd, &c, 1) > 0 && numOfBytes > 0) {
		printf("%c", c);
		--numOfBytes;
	}
	printf("\n");

	close(fd);
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
