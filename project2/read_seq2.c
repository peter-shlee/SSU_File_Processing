#include <stdio.h>
//¿¿¿¿ header file ¿¿ ¿¿

//
// argv[1]: ¿¿¿ ¿¿¿
//

#define ID_LENGTH 20
#define NAME_LENGTH 80
#define STUDENT_RECORD_SIZE (ID_LENGTH + NAME_LENGTH)

int main(int argc, char **argv)
{
	// ¿¿¿¿¿¿ ¿¿ ¿¿¿ ¿¿¿ ¿¿¿¿ ¿¿ ¿¿ ¿¿¿¿ "¿¿¿"¿¿ ¿¿¿¿¿, ¿¿
	// ¿¿¿ ¿¿¿ ¿¿¿¿ ¿¿ ¿¿¿
	struct student {
		char id[ID_LENGTH];
		char name[NAME_LENGTH];
	} student;
	off_t fileSize;
	int numOfStudents;
	int fd;
	int i;

	if(argc != 2) {
		fprintf(stderr, "usage : %s <file>\n", argv[0]);
		exit(1);
	}

	if((fd = open(argv[1], O_RDONLY)) < 0) {
		fprintf(stderr, "open error for %s\n", argv[1]);
		exit(1);
	}

	if((fileSize = lseek(fd, 0, SEEK_SET)) < 0) { // lseek¿ ¿¿¿¿ ¿¿ ¿¿¿¿¿¿ ¿¿¿ ¿ ¿¿¿¿¿ ¿¿¿ -> stat ¿¿¿ ¿¿¿ ¿¿ ¿¿¿ ¿¿¿¿
		fprintf(stderr, "lseek error\n");
		exit(1);
	}

	lseek(fd, 0, SEEK_SET);
	numOfStudents = (int)(fileSize / STUDENT_RECORD_SIZE);
	for(i = 0; i < numOfStuendts; ++i) {
		if (read(fd, &student, STUDENT_RECORD_SIZE) <= 0)
			break;
#ifdef DEBUG
		printf("id : %s, name : %s\n", student.id, student.name);
#endif
	}

	printf("#records: %d timecost: us\n" numOfStudents);

	exit(0);
}
