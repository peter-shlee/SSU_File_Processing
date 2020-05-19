#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
//필요하면 header file 추가 가능

//
// argv[1]: 레코드 파일명
//

#define STUDENT_RECORD_SIZE 100

int main(int argc, char **argv)
{
	// 표준입력으로 받은 레코드 파일에 저장되어 있는 전체 레코드를 "순차적"으로 읽어들이고, 이때
	// 걸리는 시간을 측정하는 코드 구현함
	char student[STUDENT_RECORD_SIZE];
	off_t fileSize;
	int numOfStudents;
	int fd;
	int i;
	struct timeval begin_t, end_t, interval_t;
	suseconds_t runtime;

	if(argc != 2) { // 프로그램에 전될된 인자의 개수가 올바른지 확인
		fprintf(stderr, "usage : %s <file>\n", argv[0]);
		exit(1);
	}

	if((fd = open(argv[1], O_RDONLY)) < 0) { // 파일 open
		fprintf(stderr, "open error for %s\n", argv[1]);
		exit(1);
	}

	if((fileSize = lseek(fd, 0, SEEK_END)) < 0) { // 파일의 크기를 구한다
		fprintf(stderr, "lseek error\n");
		exit(1);
	}

	numOfStudents = (int)(fileSize / STUDENT_RECORD_SIZE); // 파일에 저장되어있는 레코드의 개수를 계산

	lseek(fd, 0, SEEK_SET);
	gettimeofday(&begin_t, NULL); // 시간 측정 시작
	for(i = 0; i < numOfStudents; ++i) { // 레코드를 순차적으로 읽어 들인다
		if (read(fd, student, STUDENT_RECORD_SIZE) <= 0)
			break;
	}
	gettimeofday(&end_t, NULL); // 시간 측정 종료
	
	// read하는데 걸린 시간 계산
	interval_t.tv_sec = end_t.tv_sec - begin_t.tv_sec;
	interval_t.tv_usec = end_t.tv_usec - begin_t.tv_usec;
	if(interval_t.tv_usec < 0) {
		interval_t.tv_usec += 1000000;
		interval_t.tv_sec -= 1;
	}
	runtime = interval_t.tv_sec * 1000000 + interval_t.tv_usec;

	printf("#records: %d timecost: %ld us\n",numOfStudents, runtime); // 결과 출력

	exit(0);
}
