#include <stdio.h>
#include <string.h>
void ftl_open();
void ftl_print();
void ftl_read(int lsn, char *sectorbuf);
void ftl_write(int lsn, char *sectorbuf);

int main(){
	printf("start\n");
	char sectorData[512];

	ftl_open();
	ftl_print();
	
	sprintf(sectorData, "Hello world!! ");
	ftl_write(3, sectorData);

	memset(sectorData, 0, 512);
	ftl_read(3, sectorData);

	sprintf(sectorData, "write test");
	ftl_write(30, sectorData);

	sprintf(sectorData, "20160548 Lee Seung Hyun");
	ftl_write(43, sectorData);

	

	printf("%s\n", sectorData);
	ftl_print();

	return 0;
}
