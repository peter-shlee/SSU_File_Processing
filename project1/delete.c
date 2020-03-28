#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#define READ_WRITE_BYTES 100

long parseLong(const char *numString);

int main(int argc, char *argv[])
{
	char *buffer;
	char *entry;
	int fd;
	int length;
	int deleteBytes;
	int fileSize;
	int i;
	int offset;


	if(argc != 4){
		fprintf(stderr
			, "usage : %s <file> <offset> <delete bytes>\n"
			, argv[0]);
		exit(1);
	}

	if((fd = open(argv[1], O_RDONLY)) < 0){
		fprintf(stderr, "open error for %s\n", argv[1]);
		exit(1);
	}

	if((fileSize = lseek(fd, 0, SEEK_END)) < 0){
		fprintf(stderr, "lseek error\n");
		exit(1);
	}

	offset = parseLong(argv[2]);
	deleteBytes = parseLong(argv[3]);

	buffer = (char *)malloc(fileSize);
	if(buffer == NULL){
		fprintf(stderr, "malloc error\n");
		exit(1);
	}

	lseek(fd, 0, SEEK_SET);
	entry = buffer;
	while((length = read(fd, entry, READ_WRITE_BYTES)) > 0)
		entry += length;
	close(fd);

	if(fileSize > offset){
		if((fd = open(argv[1], O_WRONLY | O_CREAT | O_TRUNC)) < 0){
			fprintf(stderr, "open error for %s\n", argv[1]);
			exit(1);
		}
		write(fd, buffer, offset);
		if((fileSize - (offset + deleteBytes)) > 0)
			write(fd, buffer + offset + deleteBytes
				, fileSize - (offset + deleteBytes));
	}
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
