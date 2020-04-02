#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#define ID_LENGTH 20
#define NAME_LENGTH 80
#define STUDENT_RECORD_SIZE (ID_LENGTH + NAME_LENGTH)

int main(int argc, char *argv[]) 
{
	struct student {
		char id[ID_LENGTH];
		char name[NAME_LENGTH];
	} student;
	int i;
	int fd;
	char name[NAME_LENGTH];
	char id[ID_LENGTH];

	fd = open(argv[1], O_WRONLY | O_CREAT | O_TRUNC, 0644);

	strcpy(name, "test");
	for(i = 0; i < 10000; ++i) {
		name[4] = 0;
		sprintf(id, "%d", i);
		strcat(name, id);
		strcpy(student.id, id);
		strcpy(student.name, name);
		printf("id : %s, name : %s\n", student.id, student.name);
		write(fd, &student, sizeof(student));
	}

	exit(0);
}
		
		
