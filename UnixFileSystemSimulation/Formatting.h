#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include "FileSystemSimulation.h"
#define DISK_READ_FAIL 2
#define FORMAT_ERROR 1
#define FORMAT_SUCCESS 0

storage emptyStorage;

int formatDisk(char *diskfilepath)
{
	FILE *disk = fopen(diskfilepath, "rb+");
	if (disk == NULL)
	{
		printf("Open file failed!\n");
		return DISK_READ_FAIL;
	}


	bootblock bbr;
	if (fread(&bbr, sizeof(bootblock), 1, disk) != 1)
	{
		printf("Read virtual disk failed!\n");
		fclose(disk);
		return DISK_READ_FAIL;
	}

	if (bbr.verifyCode == VERIFY_CODE)
	{
		printf("Boot block detected!\n");
		printBootBlock(bbr);
		fseek(disk, 0, SEEK_SET);
		printf("Updating boot time...\n");
		bbr.lastBootTime = currentFileTime();
		if (fwrite(&bbr, sizeof(bootblock), 1, disk) != 1)
		{
			printf("Failed to format: write boot block failed.\n");
			fclose(disk);
			return FORMAT_ERROR;
		}
	}
	else
	{
		printf("Writing boot block...\n");
		fseek(disk, 0, SEEK_SET);
		bootblock bbw;
		bbw.verifyCode = VERIFY_CODE;
		bbw.bootBlockSize = BOOT_BLOCK_SIZE;
		bbw.blockSize = BLOCK_SIZE;
		bbw.diskSize = DISK_SIZE;
		bbw.superBlockSize = SUPER_BLOCK_SIZE;
		bbw.inodeNumDiv16_1 = INODE_NUM_DIV_16_1;
		bbw.createTime = currentFileTime();
		bbw.lastBootTime = currentFileTime();
		if (fwrite(&bbw, sizeof(bootblock), 1, disk) != 1)
		{
			printf("Failed to format: write boot block failed.\n");
			fclose(disk);
			return FORMAT_ERROR;
		}
	}


	superblock sb;
	sb.superBlockSize = SUPER_BLOCK_SIZE;
	sb.inodeNumDiv16_1 = INODE_NUM_DIV_16_1;
	sb.diskSize = DISK_SIZE;
	sb.capacity = DISK_SIZE - RESERVED_BLOCK_NUM;
	sb.usedcapa = 0;
	sb.avainode = (INODE_NUM_DIV_16_1 + 1) * 16 - 1;
	sb.bmstoragestartpoint = 0;
	sb.totalfilesize = 0;
	sb.createTime = currentFileTime();
	sb.lastWriteTime = currentFileTime();
	printf("Writing super block...\n");
	if (fwrite(&sb, sizeof(superblock), 1, disk) != 1)
	{
		printf("Failed to format: write super block failed.\n");
		fclose(disk);
		return FORMAT_ERROR;
	}


	bitmap bm;
	for (int i = 0; i < (DISK_SIZE - RESERVED_BLOCK_NUM) / 8; i++)
		bm._8Bits[i] = 0; //00000000
	bm._8Bits[(DISK_SIZE - RESERVED_BLOCK_NUM) / 8] = 15; //00001111
	for (int i = (DISK_SIZE - RESERVED_BLOCK_NUM) / 8 + 1; i < DISK_SIZE / 8; i++)
		bm._8Bits[i] = 255; //11111111
	printf("Writing bitmap...\n");
	if (fwrite(&bm, sizeof(bitmap), 1, disk) != 1)
	{
		printf("Failed to format: write bitmap failed.\n");
		fclose(disk);
		return FORMAT_ERROR;
	}


	inodes ins; //in[0]: i-node of root directory
	ins.in[0].mode = 1; //directory
	ins.in[0].linkNum = 1;
	ins.in[0].parentcode = 0;
	ins.in[0].fileSize = 0;
	ins.in[0].occuSize = 0;
	ins.in[0].createTime = currentFileTime();
	ins.in[0].modifyTime = currentFileTime();
	for (int i = 1; i < (INODE_NUM_DIV_16_1 + 1) * 16; i++)
		ins.in[i].linkNum = 0;
	printf("Writing i-nodes...\n");
	if (fwrite(&ins, sizeof(inodes), 1, disk) != 1)
	{
		printf("Failed to format: write i-nodes failed.\n");
		fclose(disk);
		return FORMAT_ERROR;
	}


	printf("Wiping storage...\n");
	if (fwrite(&emptyStorage, sizeof(storage), 1, disk) != 1)
	{
		printf("Failed to format: wipe storage failed.\n");
		fclose(disk);
		return FORMAT_ERROR;
	}


	fclose(disk);
	printf("Finish.\n\n");
	return FORMAT_SUCCESS;
}