#include <stdio.h>
#include <string.h>
void ftl_open();
void ftl_print();
void ftl_read(int lsn, char *sectorbuf);
void ftl_write(int lsn, char *sectorbuf);

int main(){
	char sectorData[512];

	ftl_open();
	//ftl_print();
	
	memset(sectorData, 0, 512);
	sprintf(sectorData, "0");
	ftl_write(0, sectorData);

	//ftl_print();

	memset(sectorData, 0, 512);
	sprintf(sectorData, "1");
	ftl_write(1, sectorData);

	//ftl_print();

	memset(sectorData, 0, 512);
	sprintf(sectorData, "2");
	ftl_write(2, sectorData);

	//ftl_print();

	memset(sectorData, 0, 512);
	sprintf(sectorData, "3");
	ftl_write(3, sectorData);

	//ftl_print();

	memset(sectorData, 0, 512);
	sprintf(sectorData, "new 3");
	ftl_write(3, sectorData);

	//ftl_print();

	memset(sectorData, 0, 512);
	sprintf(sectorData, "4");
	ftl_write(4, sectorData);

	//ftl_print();

	memset(sectorData, 0, 512);
	sprintf(sectorData, "5");
	ftl_write(5, sectorData);

	//ftl_print();

	memset(sectorData, 0, 512);
	sprintf(sectorData, "6");
	ftl_write(6, sectorData);

	//ftl_print();

	memset(sectorData, 0, 512);
	sprintf(sectorData, "7");
	ftl_write(7, sectorData);

	//ftl_print();

	memset(sectorData, 0, 512);
	sprintf(sectorData, "new 0");
	ftl_write(0, sectorData);

	//ftl_print();

	memset(sectorData, 0, 512);
	sprintf(sectorData, "new 4");
	ftl_write(4, sectorData);

	//ftl_print();
	
	memset(sectorData, 0, 512);
	sprintf(sectorData, "new 2");
	ftl_write(2, sectorData);

	//ftl_print();
	memset(sectorData, 0, 512);
	ftl_read(0, sectorData);
	printf("0: %s\n", sectorData);
	memset(sectorData, 0, 512);
	ftl_read(1, sectorData);
	printf("1: %s\n", sectorData);
	memset(sectorData, 0, 512);
	ftl_read(2, sectorData);
	printf("2: %s\n", sectorData);
	memset(sectorData, 0, 512);
	ftl_read(3, sectorData);
	printf("3: %s\n", sectorData);
	memset(sectorData, 0, 512);
	ftl_read(4, sectorData);
	printf("4: %s\n", sectorData);
	memset(sectorData, 0, 512);
	ftl_read(5, sectorData);
	printf("5: %s\n", sectorData);
	memset(sectorData, 0, 512);
	ftl_read(6, sectorData);
	printf("6: %s\n", sectorData);
	memset(sectorData, 0, 512);
	ftl_read(7, sectorData);
	sectorData[511] = 0;
	printf("7: %s\n", sectorData);

	return 0;
}
