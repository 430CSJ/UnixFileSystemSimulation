#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define BUFF_SIZE 1<<20
#define DISK_SIZE_MB 16 //MB
#define CREATE_SUCCESS 0
#define CREATE_FAIL 1
#define NOT_TO_CREATE 2
char buff[BUFF_SIZE];

int createDisk(char* path)
{
	FILE* disk = fopen(path, "r");
	if (disk != NULL)
	{
		fclose(disk);
		printf("%s has already exist, input Yes to overwrite it:\n", path);
		char act[4];
		scanf("%s", act);
		if (strcmp(act, "Yes") != 0)
		{
			printf("Give up creating.\n\n");
			return NOT_TO_CREATE;
		}
	}
	disk = fopen(path, "wb");
	if (disk == NULL)
	{
		printf("Create file \"%s\" failed!\n\n", path);
		return CREATE_FAIL;
	}
	printf("Creating virtual disk...   0%%");
	for (int i = 0; i < DISK_SIZE_MB; i++)
	{
		printf("\b\b\b\b");
		if (fwrite(buff, BUFF_SIZE, 1, disk) == 1)
			printf("%3d%%", (i + 1) * 100 / DISK_SIZE_MB);
		else
		{
			fclose(disk);
			printf("Failed to write \"%s\"!\n\n", path);
			return CREATE_FAIL;
		}
	}
	fclose(disk);
	printf("\nCreate virtual disk successfully: %s\n\n", path);
	return CREATE_SUCCESS;
}