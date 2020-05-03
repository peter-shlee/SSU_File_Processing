#include <stdio.h>
void ftl_open();
void ftl_print();
void ftl_read(int lsn, char *sectorbuf);

int main(){
	printf("start\n");
	char sectorData[512];

	ftl_open();
	ftl_print();
	ftl_read(3, sectorData);
	printf("%s\n", sectorData);

	return 0;
}
