#define _CRT_SECURE_NO_DEPRECATE
#include <stdio.h>
#include <windows.h>
#include "CreateDisk.h"
#include "Formatting.h"
#include "FileSystemSimulation.h"

char diskpath[300];

int main()
{
Initial:
	isdebug = false;
	printf("Welcome to i-node-based Unix-style file system simulator!\n");
	printf("Credit: Zhao Yinghao(201630022008), Yang Zhe(201636599627), Fang Yuxuan(201630438809).\n");
	printf("Input 0 to create a new disk file to simulate, 1 to open an existing disk file, other integers to exit:\n");
	scanf("%hd", &choice);
	if (choice == 0)
	{
		printf("Input the path of the file to create:\n");
		scanf("%s", diskpath);
		if (createDisk(diskpath) != CREATE_SUCCESS)
			goto Initial;
		if (formatDisk(diskpath) != FORMAT_SUCCESS)
			goto Initial;
	}
	else if (choice == 1)
	{
		printf("Input the path of the file to open:\n");
		scanf("%s", diskpath);
	}
	else
		exit(0);
	switch (fileSystemMain(diskpath))
	{
	case TO_BACK:
		goto Initial;
		break;
	case TO_FORMAT:
		formatDisk(diskpath);
		goto Initial;
		break;
	default:
		goto Initial;
		break;
	}
}