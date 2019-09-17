#pragma once
#define _CRT_SECURE_NO_DEPRECATE
#include <fstream>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <time.h>
#include <windows.h>
#define VERIFY_CODE 202
#define BLOCK_SIZE 1024 //B
#define DISK_SIZE 16384 //KB
#define BOOT_BLOCK_SIZE 1 //KB
#define SUPER_BLOCK_SIZE 1 //KB
#define INODE_SIZE 64 //B
#define INODE_NUM_DIV_16_1 255 //inode num/16-1=255, inode num=4096
#define RESERVED_BLOCK_NUM int(BOOT_BLOCK_SIZE+SUPER_BLOCK_SIZE+sizeof(bitmap)/1024+sizeof(inodes)/1024)
#define INDIRECT_ADDRESS_BLOCK_1_NUM 1
#define INDIRECT_ADDRESS_BLOCK_2_NUM 7
#define MAX_FILE_SIZE 10+INDIRECT_ADDRESS_BLOCK_1_NUM*BLOCK_SIZE/2-INDIRECT_ADDRESS_BLOCK_2_NUM+INDIRECT_ADDRESS_BLOCK_2_NUM*BLOCK_SIZE/2
#define INODE_CODE_LENGTH 2
#define MAX_FILE_NAME_LENGTH 16-INODE_CODE_LENGTH //B
#define MAX_FILE_NUM_IN_DIRE BLOCK_SIZE/16*(10+1)

#define SAVE_TO_DISK_FILE_SUCCESS 0
#define SAVE_TO_DISK_FILE_FAILED 1
#define OPEN_DISK_FILE_FAILED 2
#define READ_DISK_FILE_FAILED 3
#define NOT_THE_CORRECT_FORMAT 4
#define WRITE_DISK_FILE_FAILED 5
#define TO_FORMAT 10
#define TO_BACK 11

char cmd[300];
short choice;
bool isdebug;

struct bootblock
{
	unsigned char verifyCode;
	unsigned char bootBlockSize; //KB
	unsigned short blockSize; //B
	unsigned short diskSize; //KB
	unsigned char superBlockSize; //KB
	unsigned char inodeNumDiv16_1; //inode num/16-1
	LONGLONG createTime;
	LONGLONG lastBootTime;
	unsigned char blank[BOOT_BLOCK_SIZE * 1024 - 4 * sizeof(unsigned char) - 2 * sizeof(short) - 2 * sizeof(LONGLONG)];
};

struct superblock
{
	unsigned char superBlockSize;
	unsigned char inodeNumDiv16_1;
	unsigned short diskSize; //KB
	unsigned short capacity; //KB
	unsigned short usedcapa; //KB
	unsigned short avainode;
	unsigned short bmstoragestartpoint;
	unsigned int totalfilesize; //B
	LONGLONG createTime;
	LONGLONG lastWriteTime;
	unsigned char blank[SUPER_BLOCK_SIZE * 1024 - 2 * sizeof(unsigned char) - 5 * sizeof(unsigned short) - sizeof(unsigned int) - 2 * sizeof(LONGLONG)];
};

superblock sbtmp;

struct bitmap
{
	unsigned char _8Bits[DISK_SIZE / 8];
};

bitmap diskbm;

struct inode
{
	unsigned char mode;
	unsigned char linkNum;
	unsigned short parentcode;
	unsigned short direAddr[10];
	unsigned short indireAddr;
	unsigned short occuSize; //KB
	unsigned int fileSize; //B
	LONGLONG createTime;
	LONGLONG modifyTime;
	unsigned char blank[INODE_SIZE - 13 * sizeof(unsigned short) - 2 * sizeof(unsigned char) - sizeof(unsigned int) - 2 * sizeof(LONGLONG)];
};

struct inodes
{
	struct inode in[(INODE_NUM_DIV_16_1 + 1) * 16];
};

inodes diskins;

struct block
{
	unsigned char byte[BLOCK_SIZE];
};

struct storage
{
	struct block blocks[DISK_SIZE - RESERVED_BLOCK_NUM];
};

storage diskstor;

struct blockbyteaddress
{
	unsigned short blockaddr;
	unsigned short byteaddr;
};

struct file
{
	unsigned char filename[MAX_FILE_NAME_LENGTH];
	unsigned short inodecode;
};

struct cachestream
{
	unsigned int size;
	struct block blocks[MAX_FILE_SIZE];
};

cachestream cachebuffer;

blockbyteaddress storaddr;

blockbyteaddress* fileAddr2StorageAddr(file* f, unsigned int fileaddr)
{
	if (f == nullptr || fileaddr >= diskins.in[f->inodecode].fileSize)
		return nullptr;
	storaddr.byteaddr = fileaddr % BLOCK_SIZE;
	unsigned short fileblocknum = fileaddr >> 10;
	if (fileblocknum < 10)
		storaddr.blockaddr = diskins.in[f->inodecode].direAddr[fileblocknum];
	else if (diskins.in[f->inodecode].mode == 1)
		storaddr.blockaddr = diskins.in[f->inodecode].indireAddr;
	else if (fileblocknum < 10 + BLOCK_SIZE / 2 - INDIRECT_ADDRESS_BLOCK_2_NUM)
	{
		blockbyteaddress addrblock1;
		addrblock1.blockaddr = diskins.in[f->inodecode].indireAddr;
		addrblock1.byteaddr = (fileblocknum - 10) * 2;
		block* addrblock = &(diskstor.blocks[addrblock1.blockaddr]);
		storaddr.blockaddr = addrblock->byte[addrblock1.byteaddr] + (addrblock->byte[addrblock1.byteaddr + 1] << 8);
	}
	else if (fileblocknum < 10 + BLOCK_SIZE / 2 - INDIRECT_ADDRESS_BLOCK_2_NUM + INDIRECT_ADDRESS_BLOCK_2_NUM*BLOCK_SIZE / 2)
	{
		blockbyteaddress addrblock1;
		addrblock1.blockaddr = diskins.in[f->inodecode].indireAddr;
		addrblock1.byteaddr = (fileblocknum - (10 + BLOCK_SIZE / 2 - INDIRECT_ADDRESS_BLOCK_2_NUM)) / (BLOCK_SIZE / 2) + BLOCK_SIZE - INDIRECT_ADDRESS_BLOCK_2_NUM * 2;
		block* indireblock = &(diskstor.blocks[addrblock1.blockaddr]);
		blockbyteaddress addrblock2;
		addrblock2.blockaddr = indireblock->byte[addrblock1.byteaddr] + (indireblock->byte[addrblock1.byteaddr + 1] << 8);
		addrblock2.byteaddr = (fileblocknum - (10 + BLOCK_SIZE / 2 - INDIRECT_ADDRESS_BLOCK_2_NUM)) % (BLOCK_SIZE / 2);
		block* addrblock = &(diskstor.blocks[addrblock2.blockaddr]);
		storaddr.blockaddr = addrblock->byte[addrblock2.byteaddr] + (addrblock->byte[addrblock2.byteaddr + 1] << 8);
	}
	else
		return nullptr;
	return &storaddr;
}

void printByteValue(blockbyteaddress* bba)
{
	if (bba != nullptr)
		if (bba->blockaddr < DISK_SIZE - RESERVED_BLOCK_NUM)
			if (bba->byteaddr < BLOCK_SIZE)
				printf("%c", diskstor.blocks[bba->blockaddr].byte[bba->byteaddr]);
}

void writeByteValue(blockbyteaddress* bba, const char c)
{
	if (bba != nullptr)
		if (bba->blockaddr < DISK_SIZE - RESERVED_BLOCK_NUM)
			if (bba->byteaddr < BLOCK_SIZE)
				diskstor.blocks[bba->blockaddr].byte[bba->byteaddr] = c;
}

unsigned short firstAvaBlockP, lastAvaBlockP;

void refreshAvaBlockP()
{
	unsigned short rest = (DISK_SIZE - RESERVED_BLOCK_NUM) % 8;
	bool getFirst = false, getLast = false;
	for (unsigned short i = 0; i < (DISK_SIZE - RESERVED_BLOCK_NUM) / 8; i++)
	{
		for (unsigned short j = 7; j >= 0; j--)
			if (((diskbm._8Bits[i] >> j) & 1) == 0)
			{
				firstAvaBlockP = i * 8 + 7 - j;
				getFirst = true;
				break;
			}
		if (getFirst)
			break;
	}
	for (unsigned short i=8-rest;i<8;i++)
		if (((diskbm._8Bits[(DISK_SIZE - RESERVED_BLOCK_NUM) / 8] >> i) & 1) == 0)
		{
			lastAvaBlockP = DISK_SIZE - RESERVED_BLOCK_NUM - 1 + 8 - rest - i;
			getLast = true;
			break;
		}
	if (!getLast)
		for (unsigned short i = (DISK_SIZE - RESERVED_BLOCK_NUM) / 8 - 1; i >= 0; i--)
		{
			for (unsigned short j=0;j<8;j++)
				if (((diskbm._8Bits[i] >> j) & 1) == 0)
				{
					lastAvaBlockP = i * 8 + 7 - j;
					getLast = true;
					break;
				}
			if (getLast)
				break;
		}
}

void setBit(unsigned short blockno, unsigned short val)
{
	if (blockno < DISK_SIZE - RESERVED_BLOCK_NUM)
	{
		unsigned short _8bitno = (blockno + 1) / 8 + ((blockno + 1) % 8 > 0) - 1;
		if (val == 0)
			diskbm._8Bits[_8bitno] = diskbm._8Bits[_8bitno] & (255 - (1 << (7 - blockno % 8)));
		else if (val == 1)
			diskbm._8Bits[_8bitno] = diskbm._8Bits[_8bitno] | (1 << (7 - blockno % 8));
	}
}

unsigned short firstAvaInodeP = 0;

void refreshFirstAvaInodeP()
{
	unsigned short availinodenum = 0;
	bool firstavailinodefound = false;
	for (unsigned short i=0;i<(INODE_NUM_DIV_16_1+1)*16;i++)
		if (diskins.in[i].linkNum == 0)
		{
			availinodenum++;
			if (!firstavailinodefound)
			{
				firstAvaInodeP = i;
				firstavailinodefound = true;
			}
		}
	sbtmp.avainode = availinodenum;
}

file currentfile;

struct fileentry
{
	char filename[MAX_FILE_NAME_LENGTH];
	unsigned short inodecode;
};

struct directory
{
	unsigned char directoryname[MAX_FILE_NAME_LENGTH];
	unsigned short filenum;
	unsigned short inodecode;
	struct fileentry filelist[MAX_FILE_NUM_IN_DIRE];
};

directory currentdire;

void MillisecondSince1970ToSystemTime(LONGLONG nMillisecond, SYSTEMTIME *lpstTime)
{
	LARGE_INTEGER liTime;
	liTime.QuadPart = nMillisecond;
	// 先转换为100微秒单位的FILETIME
	liTime.QuadPart *= 10000;
	// FILETIME是1960年开始的，需要和1970年进行转换
	liTime.QuadPart += 116444736000000000;
	// 转换成SYSTEMTIME
	::FileTimeToSystemTime((LPFILETIME)&liTime, lpstTime);
}

LONGLONG currentFileTime()
{
	LONGLONG nFileTime;
	SYSTEMTIME stTime;
	::GetLocalTime(&stTime);
	::SystemTimeToFileTime(&stTime, (LPFILETIME)&nFileTime);
	return nFileTime;
}

void printFileTimeAsSystemTime(LONGLONG nFileTime)
{
	SYSTEMTIME stTime;
	nFileTime -= 116444736000000000;
	nFileTime /= 10000;

	// 调用转换函数
	MillisecondSince1970ToSystemTime(nFileTime, &stTime);

	// 输出字符串
	::printf("%04d-%02d-%02d %02d:%02d:%02d",
		stTime.wYear, stTime.wMonth, stTime.wDay,
		stTime.wHour, stTime.wMinute, stTime.wSecond);
}

#define FILE_PRINTED 0
#define FILE_NOT_EXIST 1

int printFile(file* f)
{
	if (f != nullptr)
	{
		if (diskins.in[f->inodecode].linkNum < 1)
			return FILE_NOT_EXIST;
		if (isdebug)
			printf("Inode code: %hd\n", f->inodecode);
		blockbyteaddress* currentblockaddr;
		for (unsigned int i = 0; i < diskins.in[f->inodecode].fileSize / BLOCK_SIZE; i++)
		{
			currentblockaddr = fileAddr2StorageAddr(f, i*BLOCK_SIZE);
			if (currentblockaddr != nullptr)
				for (currentblockaddr->byteaddr = 0; currentblockaddr->byteaddr < BLOCK_SIZE; currentblockaddr->byteaddr++)
					printByteValue(currentblockaddr);
		}
		currentblockaddr = fileAddr2StorageAddr(f, diskins.in[f->inodecode].fileSize - 1);
		if (currentblockaddr != nullptr)
			for (currentblockaddr->byteaddr = 0; currentblockaddr->byteaddr < diskins.in[f->inodecode].fileSize%BLOCK_SIZE; currentblockaddr->byteaddr++)
				printByteValue(currentblockaddr);
		return FILE_PRINTED;
	}
	else
		return FILE_NOT_EXIST;
}

unsigned short calcFileOccuCapa(unsigned int size) //KB
{
	unsigned short occublocknum = size / BLOCK_SIZE + (size%BLOCK_SIZE > 0);
	if (occublocknum <= 10)
		return occublocknum;
	else if (occublocknum <= 10 + BLOCK_SIZE / 2 - INDIRECT_ADDRESS_BLOCK_2_NUM)
		return occublocknum + 1;
	else if (occublocknum <= MAX_FILE_SIZE)
		return occublocknum + 1 + (occublocknum - (10 + BLOCK_SIZE / 2 - INDIRECT_ADDRESS_BLOCK_2_NUM)) / (BLOCK_SIZE / 2) + ((occublocknum - (10 + BLOCK_SIZE / 2 - INDIRECT_ADDRESS_BLOCK_2_NUM)) % (BLOCK_SIZE / 2) > 0);
	else
		return 0;
}

#define DIRE_READ 0
#define DIRE_PRINTED 0
#define NOT_A_DIRE 2

int readDirectory(directory* dire)
{
	if (dire != nullptr)
	{
		if (diskins.in[dire->inodecode].linkNum > 0)
		{
			if (diskins.in[dire->inodecode].mode == 1)
			{
				dire->filenum = diskins.in[dire->inodecode].fileSize / sizeof(fileentry);
				blockbyteaddress* currentblockaddr;
				unsigned int fileno = 0;
				file direfile;
				direfile.inodecode = dire->inodecode;
				for (unsigned int i = 0; i < diskins.in[dire->inodecode].fileSize / BLOCK_SIZE; i++)
				{
					currentblockaddr = fileAddr2StorageAddr(&direfile, i*BLOCK_SIZE);
					if (currentblockaddr!=nullptr)
						for (currentblockaddr->byteaddr = 0; currentblockaddr->byteaddr < BLOCK_SIZE; currentblockaddr->byteaddr += sizeof(fileentry))
						{
							unsigned short j = 0;
							do
								dire->filelist[fileno].filename[j] = diskstor.blocks[currentblockaddr->blockaddr].byte[currentblockaddr->byteaddr + j];
							while (dire->filelist[fileno].filename[j++] != '\0' && j < MAX_FILE_NAME_LENGTH);
							dire->filelist[fileno++].inodecode = diskstor.blocks[currentblockaddr->blockaddr].byte[currentblockaddr->byteaddr + MAX_FILE_NAME_LENGTH] + (diskstor.blocks[currentblockaddr->blockaddr].byte[currentblockaddr->byteaddr + MAX_FILE_NAME_LENGTH + 1] << 8);
						}
				}
				if (diskins.in[dire->inodecode].fileSize%BLOCK_SIZE > 0)
				{
					currentblockaddr = fileAddr2StorageAddr(&direfile, diskins.in[dire->inodecode].fileSize - sizeof(fileentry));
					if (currentblockaddr != nullptr)
						for (currentblockaddr->byteaddr = 0; currentblockaddr->byteaddr < diskins.in[dire->inodecode].fileSize%BLOCK_SIZE; currentblockaddr->byteaddr += sizeof(fileentry))
						{
							unsigned short j = 0;
							do
								dire->filelist[fileno].filename[j] = diskstor.blocks[currentblockaddr->blockaddr].byte[currentblockaddr->byteaddr + j];
							while (dire->filelist[fileno].filename[j++] != '\0' && j < MAX_FILE_NAME_LENGTH);
							dire->filelist[fileno++].inodecode = diskstor.blocks[currentblockaddr->blockaddr].byte[currentblockaddr->byteaddr + MAX_FILE_NAME_LENGTH] + (diskstor.blocks[currentblockaddr->blockaddr].byte[currentblockaddr->byteaddr + MAX_FILE_NAME_LENGTH + 1] << 8);
						}
				}
				return DIRE_READ;
			}
			else
				return NOT_A_DIRE;
		}
		else
			return FILE_NOT_EXIST;
	}
	else
		return FILE_NOT_EXIST;
}

int printDirectory(directory* dire)
{
	int readresult = readDirectory(dire);
	if (readresult == DIRE_READ)
	{
		if (isdebug)
			printf("Inode code: %hd\n", dire->inodecode);
		if (dire->filenum == 0)
			printf("Empty Directory.\n");
		else
			printf("Name             Size(B)  OccupiedSpace(KB)  CreateTime           LastModifyTime\n");
		for (unsigned short i = 0; i < dire->filenum; i++)
		{
			unsigned short j = 0;
			while (j < MAX_FILE_NAME_LENGTH && dire->filelist[i].filename[j] != '\0')
				printf("%c", dire->filelist[i].filename[j++]);
			if (diskins.in[dire->filelist[i].inodecode].mode == 1)
			{
				printf("/");
				j++;
			}
			for (; j < MAX_FILE_NAME_LENGTH + 1 + 2; j++)
				printf(" ");
			printf("%7u", diskins.in[dire->filelist[i].inodecode].fileSize);
			printf("%19hd  ", diskins.in[dire->filelist[i].inodecode].occuSize);
			printFileTimeAsSystemTime(diskins.in[dire->filelist[i].inodecode].createTime);
			printf("  ");
			printFileTimeAsSystemTime(diskins.in[dire->filelist[i].inodecode].modifyTime);
			if (isdebug)
				printf(" %hd", dire->filelist[i].inodecode);
			printf("\n");
		}
		return DIRE_PRINTED;
	}
	else
		return readresult;
}

#define FILE_NOT_FOUND -1

short findFile(directory* parentdire, char* targetname)
{
	if (readDirectory(parentdire) == DIRE_READ)
		for (unsigned short i = 0; i < parentdire->filenum; i++)
		{
			for (unsigned short j = 0; j < MAX_FILE_NAME_LENGTH; j++)
			{
				if (targetname[j] != parentdire->filelist[i].filename[j])
					break;
				else if (targetname[j] == '\0' || j == MAX_FILE_NAME_LENGTH - 1)
					return i;
			}
		}
	return FILE_NOT_FOUND;

}

#define FILE_CREATE_SUCCESS 0
#define INVALID_DIRE 1
#define DIRE_FULL 2
#define NOT_ENOUGH_SPACE 3
#define ADDRESS_ERROR 4
#define SAME_NAME_FILE_EXIST 5
#define SIZE_OVER_LIMIT 6
#define NO_AVAIL_INODE 7

int createFile(directory* parentdire, char* name, unsigned short filemode)
{
	if (parentdire != nullptr)
	{
		if (cachebuffer.size > MAX_FILE_SIZE*BLOCK_SIZE)
			return SIZE_OVER_LIMIT;
		if (readDirectory(parentdire) != DIRE_READ)
			return INVALID_DIRE;
		if (sbtmp.avainode == 0)
			return NO_AVAIL_INODE;
		if (diskins.in[parentdire->inodecode].linkNum > 0)
		{
			if (parentdire->filenum < MAX_FILE_NUM_IN_DIRE)
			{
				if (findFile(parentdire, name) != FILE_NOT_FOUND)
					return SAME_NAME_FILE_EXIST;
				diskins.in[firstAvaInodeP].mode = filemode;
				unsigned short spacerequired = (diskins.in[firstAvaInodeP].mode == 0 ? calcFileOccuCapa(cachebuffer.size) : 0) + (diskins.in[parentdire->inodecode].fileSize%BLOCK_SIZE == 0);
				if (spacerequired < sbtmp.capacity - sbtmp.usedcapa)
				{
					if (diskins.in[firstAvaInodeP].mode == 0)
					{
						diskins.in[firstAvaInodeP].fileSize = cachebuffer.size;
						diskins.in[firstAvaInodeP].occuSize = calcFileOccuCapa(cachebuffer.size);
						file filetocreate;
						filetocreate.inodecode = firstAvaInodeP;
						for (unsigned short i = 0; i < (calcFileOccuCapa(cachebuffer.size) <= 10 ? calcFileOccuCapa(cachebuffer.size) : 10); i++)
						{
							diskins.in[firstAvaInodeP].direAddr[i] = firstAvaBlockP;
							setBit(firstAvaBlockP, 1);
							refreshAvaBlockP();
						}
						if (calcFileOccuCapa(cachebuffer.size) > 10)
						{
							diskins.in[firstAvaInodeP].indireAddr = firstAvaBlockP;
							setBit(firstAvaBlockP, 1);
							refreshAvaBlockP();
						}
						for (unsigned short i = 10; i < (calcFileOccuCapa(cachebuffer.size) <= 10 + BLOCK_SIZE / 2 - INDIRECT_ADDRESS_BLOCK_2_NUM ? calcFileOccuCapa(cachebuffer.size) : 10 + BLOCK_SIZE / 2 - INDIRECT_ADDRESS_BLOCK_2_NUM); i++)
						{
							diskstor.blocks[diskins.in[firstAvaInodeP].indireAddr].byte[(i - 10) * 2 + 1] = firstAvaBlockP >> 8;
							diskstor.blocks[diskins.in[firstAvaInodeP].indireAddr].byte[(i - 10) * 2] = firstAvaBlockP & 255;
							setBit(firstAvaBlockP, 1);
							refreshAvaBlockP();
						}
						unsigned short indireblock2addr;
						for (unsigned short i = 10 + BLOCK_SIZE / 2 - INDIRECT_ADDRESS_BLOCK_2_NUM; i < calcFileOccuCapa(cachebuffer.size); i++)
						{
							if ((i - (10 + BLOCK_SIZE / 2 - INDIRECT_ADDRESS_BLOCK_2_NUM)) % (BLOCK_SIZE / 2) == 0)
							{
								diskstor.blocks[diskins.in[firstAvaInodeP].indireAddr].byte[BLOCK_SIZE - INDIRECT_ADDRESS_BLOCK_2_NUM * 2 + (i - (10 + BLOCK_SIZE / 2 - INDIRECT_ADDRESS_BLOCK_2_NUM)) / (BLOCK_SIZE / 2) * 2 + 1] = firstAvaBlockP >> 8;
								diskstor.blocks[diskins.in[firstAvaInodeP].indireAddr].byte[BLOCK_SIZE - INDIRECT_ADDRESS_BLOCK_2_NUM * 2 + (i - (10 + BLOCK_SIZE / 2 - INDIRECT_ADDRESS_BLOCK_2_NUM)) / (BLOCK_SIZE / 2) * 2] = firstAvaBlockP & 255;
								indireblock2addr = firstAvaBlockP;
								setBit(firstAvaBlockP, 1);
								refreshAvaBlockP();
							}
							diskstor.blocks[indireblock2addr].byte[((i - (10 + BLOCK_SIZE / 2 - INDIRECT_ADDRESS_BLOCK_2_NUM)) % (BLOCK_SIZE / 2)) * 2 + 1] = firstAvaBlockP >> 8;
							diskstor.blocks[indireblock2addr].byte[((i - (10 + BLOCK_SIZE / 2 - INDIRECT_ADDRESS_BLOCK_2_NUM)) % (BLOCK_SIZE / 2)) * 2] = firstAvaBlockP & 255;
							setBit(firstAvaBlockP, 1);
							refreshAvaBlockP();
						}
						for (unsigned int i = 0; i < cachebuffer.size; i++)
						{
							blockbyteaddress* bbac = fileAddr2StorageAddr(&filetocreate, i);
							if (bbac != nullptr)
								diskstor.blocks[bbac->blockaddr].byte[bbac->byteaddr] = cachebuffer.blocks[i / BLOCK_SIZE].byte[i%BLOCK_SIZE];
							else
								return ADDRESS_ERROR;
						}
					}
					else if (diskins.in[firstAvaInodeP].mode == 1)
					{
						diskins.in[firstAvaInodeP].fileSize = 0;
						diskins.in[firstAvaInodeP].occuSize = 0;
					}
					parentdire->filenum++;
					unsigned short namep0 = 0;
					do
						parentdire->filelist[parentdire->filenum - 1].filename[namep0] = name[namep0];
					while (name[namep0++] != '\0' && namep0 < MAX_FILE_NAME_LENGTH);
					parentdire->filelist[parentdire->filenum - 1].inodecode = firstAvaInodeP;
					if (diskins.in[parentdire->inodecode].fileSize%BLOCK_SIZE == 0)
					{
						if (diskins.in[parentdire->inodecode].fileSize / BLOCK_SIZE < 10)
						{
							diskins.in[parentdire->inodecode].direAddr[diskins.in[parentdire->inodecode].fileSize / BLOCK_SIZE] = lastAvaBlockP;
							diskins.in[parentdire->inodecode].occuSize = diskins.in[parentdire->inodecode].fileSize / BLOCK_SIZE + 1;
							setBit(lastAvaBlockP, 1);
							refreshAvaBlockP();
						}
						else if (diskins.in[parentdire->inodecode].fileSize / BLOCK_SIZE == 10)
						{
							diskins.in[parentdire->inodecode].indireAddr = lastAvaBlockP;
							diskins.in[parentdire->inodecode].occuSize = 11;
							setBit(lastAvaBlockP, 1);
							refreshAvaBlockP();
						}
					}
					else
						diskins.in[parentdire->inodecode].occuSize = diskins.in[parentdire->inodecode].fileSize / BLOCK_SIZE + 1;
					file pdire;
					pdire.inodecode = parentdire->inodecode;
					diskins.in[parentdire->inodecode].fileSize += sizeof(fileentry);
					blockbyteaddress* bbaf = fileAddr2StorageAddr(&pdire, diskins.in[parentdire->inodecode].fileSize - sizeof(fileentry));
					if (bbaf != nullptr)
					{
						unsigned short namep = 0;
						do
							diskstor.blocks[bbaf->blockaddr].byte[bbaf->byteaddr + namep] = name[namep];
						while (name[namep++] != '\0' && namep < MAX_FILE_NAME_LENGTH);
						diskstor.blocks[bbaf->blockaddr].byte[bbaf->byteaddr + MAX_FILE_NAME_LENGTH + 1] = firstAvaInodeP >> 8;
						diskstor.blocks[bbaf->blockaddr].byte[bbaf->byteaddr + MAX_FILE_NAME_LENGTH] = firstAvaInodeP % 255;
						diskins.in[parentdire->inodecode].modifyTime = currentFileTime();
						diskins.in[firstAvaInodeP].parentcode = parentdire->inodecode;
						diskins.in[firstAvaInodeP].linkNum++;
						diskins.in[firstAvaInodeP].createTime = currentFileTime();
						diskins.in[firstAvaInodeP].modifyTime = currentFileTime();
					}
					else
						return ADDRESS_ERROR;
					refreshFirstAvaInodeP();
					sbtmp.lastWriteTime = currentFileTime();
					return FILE_CREATE_SUCCESS;
				}
				else
					return NOT_ENOUGH_SPACE;
			}
			else
				return DIRE_FULL;
		}
		else
			return INVALID_DIRE;
	}
	else
		return INVALID_DIRE;
}

#define FILE_DELETED 0
#define FILE_DELETED_FAILED 2

int deleteFile(directory *parentdire, char *targetfilename) //only used to deleted a file, not a directory
{
	if (parentdire != nullptr)
	{
		if (readDirectory(parentdire) != DIRE_READ)
			return FILE_NOT_EXIST;
		short targetno = findFile(parentdire, targetfilename);
		if (targetno == FILE_NOT_FOUND)
			return FILE_NOT_EXIST;
		unsigned short targetinodecodde = parentdire->filelist[targetno].inodecode;
		if (diskins.in[targetinodecodde].mode > 0)
			return FILE_DELETED_FAILED;
		file parent;
		parent.inodecode = parentdire->inodecode;
		for (unsigned short i = targetno; i < parentdire->filenum - 1; i++)
		{
			parentdire->filelist[i].inodecode = parentdire->filelist[i + 1].inodecode;
			unsigned short j = 0;
			do
				parentdire->filelist[i].filename[j] = parentdire->filelist[i + 1].filename[j];
			while (parentdire->filelist[i].filename[j++] != '\0' && j < MAX_FILE_NAME_LENGTH);
			blockbyteaddress* bba = fileAddr2StorageAddr(&parent, i * sizeof(fileentry));
			if (bba != nullptr)
			{
				j = 0;
				do
					diskstor.blocks[bba->blockaddr].byte[bba->byteaddr + j] = parentdire->filelist[i].filename[j];
				while (parentdire->filelist[i].filename[j++] != '\0' && j < MAX_FILE_NAME_LENGTH);
				diskstor.blocks[bba->blockaddr].byte[bba->byteaddr + MAX_FILE_NAME_LENGTH + 1] = parentdire->filelist[i].inodecode >> 8;
				diskstor.blocks[bba->blockaddr].byte[bba->byteaddr + MAX_FILE_NAME_LENGTH] = parentdire->filelist[i].inodecode & 255;
			}
			else
				return FILE_DELETED_FAILED;
		}
		if (diskins.in[parentdire->inodecode].fileSize%BLOCK_SIZE > 0 && diskins.in[parentdire->inodecode].fileSize%BLOCK_SIZE <= sizeof(fileentry))
		{
			if (diskins.in[parentdire->inodecode].fileSize / BLOCK_SIZE < 10)
				setBit(diskins.in[parentdire->inodecode].direAddr[diskins.in[parentdire->inodecode].fileSize / BLOCK_SIZE], 0);
			else
				setBit(diskins.in[parentdire->inodecode].indireAddr, 0);
			refreshAvaBlockP();
		}
		diskins.in[parentdire->inodecode].fileSize -= sizeof(fileentry);
		diskins.in[parentdire->inodecode].occuSize = diskins.in[parentdire->inodecode].fileSize / BLOCK_SIZE + (diskins.in[parentdire->inodecode].fileSize%BLOCK_SIZE != 0);
		diskins.in[parentdire->inodecode].modifyTime = currentFileTime();
		diskins.in[targetinodecodde].linkNum > 0 ? diskins.in[targetinodecodde].linkNum-- : diskins.in[targetinodecodde].linkNum = 0;
		file filetodelete;
		filetodelete.inodecode = targetinodecodde;
		for (unsigned int i = 0; i < diskins.in[targetinodecodde].fileSize; i += BLOCK_SIZE)
		{
			blockbyteaddress* bba = fileAddr2StorageAddr(&filetodelete, i);
			if (bba != nullptr)
				setBit(bba->blockaddr, 0);
			else
				return FILE_DELETED_FAILED;
		}
		refreshAvaBlockP();
		refreshFirstAvaInodeP();
		return FILE_DELETED;
	}
	else
		return FILE_NOT_EXIST;
}

#define DIRE_DELETED 0
#define DIRE_DELETED_FAILED 2

int deleteDire(directory* parentdire, char* targetdirename)
{
	if (parentdire == nullptr)
		return FILE_NOT_EXIST;
	if (readDirectory(parentdire) != DIRE_READ)
		return FILE_NOT_EXIST;
	short targetno = findFile(parentdire, targetdirename);
	if (targetno == FILE_NOT_FOUND)
		return FILE_NOT_EXIST;
	unsigned short targetinodecode = parentdire->filelist[targetno].inodecode;
	directory targetdire;
	targetdire.inodecode = targetinodecode;
	if (readDirectory(&targetdire) != DIRE_READ)
		return DIRE_DELETED_FAILED;
	file parent;
	parent.inodecode = parentdire->inodecode;
	for (unsigned short i = targetno; i < parentdire->filenum - 1; i++)
	{
		parentdire->filelist[i].inodecode = parentdire->filelist[i + 1].inodecode;
		unsigned short j = 0;
		do
			parentdire->filelist[i].filename[j] = parentdire->filelist[i + 1].filename[j];
		while (parentdire->filelist[i].filename[j++] != '\0' && j < MAX_FILE_NAME_LENGTH);
		blockbyteaddress* bba = fileAddr2StorageAddr(&parent, i * sizeof(fileentry));
		if (bba != nullptr)
		{
			j = 0;
			do
				diskstor.blocks[bba->blockaddr].byte[bba->byteaddr + j] = parentdire->filelist[i].filename[j];
			while (parentdire->filelist[i].filename[j++] != '\0' && j < MAX_FILE_NAME_LENGTH);
			diskstor.blocks[bba->blockaddr].byte[bba->byteaddr + MAX_FILE_NAME_LENGTH + 1] = parentdire->filelist[i].inodecode >> 8;
			diskstor.blocks[bba->blockaddr].byte[bba->byteaddr + MAX_FILE_NAME_LENGTH] = parentdire->filelist[i].inodecode & 255;
		}
		else
			return FILE_DELETED_FAILED;
	}
	if (diskins.in[parentdire->inodecode].fileSize%BLOCK_SIZE > 0 && diskins.in[parentdire->inodecode].fileSize%BLOCK_SIZE <= sizeof(fileentry))
	{
		if (diskins.in[parentdire->inodecode].fileSize / BLOCK_SIZE < 10)
			setBit(diskins.in[parentdire->inodecode].direAddr[diskins.in[parentdire->inodecode].fileSize / BLOCK_SIZE], 0);
		else
			setBit(diskins.in[parentdire->inodecode].indireAddr, 0);
		refreshAvaBlockP();
	}
	diskins.in[parentdire->inodecode].fileSize -= sizeof(fileentry);
	diskins.in[parentdire->inodecode].occuSize = diskins.in[parentdire->inodecode].fileSize / BLOCK_SIZE + (diskins.in[parentdire->inodecode].fileSize%BLOCK_SIZE != 0);
	diskins.in[parentdire->inodecode].modifyTime = currentFileTime();
	if (targetdire.filenum>0)
		for (short i = targetdire.filenum - 1; i >= 0; i--)
		{
			char direfilename[MAX_FILE_NAME_LENGTH];
			unsigned short j = 0;
			do
				direfilename[j] = targetdire.filelist[i].filename[j];
			while (direfilename[j++] != '\0' && j < MAX_FILE_NAME_LENGTH);
			if (diskins.in[targetdire.filelist[i].inodecode].mode == 0)
				deleteFile(&targetdire, direfilename);
			else if (diskins.in[targetdire.filelist[i].inodecode].mode == 1)
				deleteDire(&targetdire, direfilename);
		}
	diskins.in[targetdire.inodecode].linkNum > 0 ? diskins.in[targetdire.inodecode].linkNum-- : diskins.in[targetdire.inodecode].linkNum = 0;
	refreshFirstAvaInodeP();
	return DIRE_DELETED;
}

void printBootBlock(bootblock bb)
{
	printf("Block size: %huB\n", bb.blockSize);
	printf("Disk size: %huKB\n", bb.diskSize);
	printf("Boot block size: %uKB\n", bb.bootBlockSize);
	printf("Disk create time: ");
	printFileTimeAsSystemTime(bb.createTime);
	printf("\nDisk last boot time: ");
	printFileTimeAsSystemTime(bb.lastBootTime);
	printf("\n\n");
}

void printSuperBlock(superblock sb)
{
	printf("Super block size: %huKB\n", sb.superBlockSize);
	printf("Capacity: %huKB\n", sb.capacity);
	printf("Used: %huKB\n", sb.usedcapa);
	printf("Available: %huKB\n", sb.capacity - sb.usedcapa);
	printf("Total file size: %uB\n", sb.totalfilesize);
	printf("I-node number: %hu\n", (sb.inodeNumDiv16_1 + 1) * 16);
	printf("Available I-node number: %hu\n", sb.avainode);
	printf("Start point in bitmap of storage: %hu\n", sb.bmstoragestartpoint);
	printf("File system create time: ");
	printFileTimeAsSystemTime(sb.createTime);
	printf("\nFile system last write time: ");
	printFileTimeAsSystemTime(sb.lastWriteTime);
	printf("\n\n");
}

void refreshUsedCapatmp()
{
	unsigned short usedcapatmp = 0;
	for (unsigned short i = 0; i < DISK_SIZE - RESERVED_BLOCK_NUM; i++)
		if (((diskbm._8Bits[i / 8] >> (7 - (i % 8))) & 1) == 1)
			usedcapatmp++;
	sbtmp.usedcapa = usedcapatmp;
}

void refreshTotalFileSizetmp()
{
	unsigned int totalfizesizetmp = 0;
	unsigned short avaInodes = 0;
	for (unsigned short i = 0; i < (INODE_NUM_DIV_16_1 + 1) * 16; i++)
		if (diskins.in[i].linkNum > 0)
			totalfizesizetmp += diskins.in[i].fileSize;
		else
			avaInodes++;
	sbtmp.totalfilesize = totalfizesizetmp;
	sbtmp.avainode = avaInodes;
}

int fileSystemMain(char* diskPath)
{
	printf("Opening virtual disk file...\n");
	FILE* disk = fopen(diskPath, "rb+");
	if (disk == NULL)
	{
		printf("Open virtual disk file failed!\n\n");
		return OPEN_DISK_FILE_FAILED;
	}


	bootblock bbr;
	fseek(disk, 0, SEEK_SET);
	if (fread(&bbr, sizeof(bootblock), 1, disk) != 1)
	{
		printf("Read virtual disk file failed!\n\n");
		fclose(disk);
		return READ_DISK_FILE_FAILED;
	}
	if (bbr.verifyCode != VERIFY_CODE)
	{
		printf("Boot block not detected. The file is not a virtual disk file or is corrupted.\n\n");
		fclose(disk);
		return NOT_THE_CORRECT_FORMAT;
	}
	printf("Boot block detected! Loading it in the memory...\n");
	printBootBlock(bbr);
	fseek(disk, 0, SEEK_SET);
	printf("Updating boot time in disk file...\n\n");
	bbr.lastBootTime = currentFileTime();
	if (fwrite(&bbr, sizeof(bootblock), 1, disk) != 1)
	{
		printf("Write virtual disk file failed!\n\n");
		fclose(disk);
		return WRITE_DISK_FILE_FAILED;
	}


	printf("Reading super block...\n");
	superblock sb;
	fseek(disk, BOOT_BLOCK_SIZE*BLOCK_SIZE, SEEK_SET);
	if (fread(&sb, sizeof(superblock), 1, disk) != 1)
	{
		printf("Read super block failed!\n\n");
		fclose(disk);
		return READ_DISK_FILE_FAILED;
	}
	printf("Loading it in the memory...\n");
	printSuperBlock(sb);


	printf("Input 0 to load this virtual disk into memory, 2 to format this disk, other integers to close the disk:\n");
	scanf("%hd", &choice);
	if (choice == 0)
	{
		printf("Reading bitmap and loading it in memory...\n");
		fseek(disk, (BOOT_BLOCK_SIZE + SUPER_BLOCK_SIZE)*BLOCK_SIZE, SEEK_SET);
		if (fread(&diskbm, sizeof(bitmap), 1, disk) != 1)
		{
			printf("Read bitmap failed!\n\n");
			fclose(disk);
			return READ_DISK_FILE_FAILED;
		}
		refreshAvaBlockP();


		printf("Reading i-nodes and loading it in memory...\n");
		fseek(disk, (BOOT_BLOCK_SIZE + SUPER_BLOCK_SIZE)*BLOCK_SIZE + sizeof(bitmap), SEEK_SET);
		if (fread(&diskins, sizeof(inodes), 1, disk) != 1)
		{
			printf("Read inodes failed!\n\n");
			fclose(disk);
			return READ_DISK_FILE_FAILED;
		}
		refreshFirstAvaInodeP();


		printf("Reading storage and loading it in memory...\n");
		fseek(disk, (BOOT_BLOCK_SIZE + SUPER_BLOCK_SIZE)*BLOCK_SIZE + sizeof(bitmap) + sizeof(inodes), SEEK_SET);
		if (fread(&diskstor, sizeof(storage), 1, disk) != 1)
		{
			printf("Read storage failed!\n\n");
			fclose(disk);
			return READ_DISK_FILE_FAILED;
		}


		sbtmp.superBlockSize = sb.superBlockSize;
		sbtmp.inodeNumDiv16_1 = sb.inodeNumDiv16_1;
		sbtmp.diskSize = sb.diskSize;
		sbtmp.capacity = sb.capacity;
		sbtmp.usedcapa = sb.usedcapa;
		sbtmp.createTime = sb.createTime;
		sbtmp.avainode = sb.avainode;
		sbtmp.bmstoragestartpoint = sb.bmstoragestartpoint;
		sbtmp.totalfilesize = sb.totalfilesize;
		sbtmp.lastWriteTime = sb.lastWriteTime;
		refreshUsedCapatmp();
		refreshTotalFileSizetmp();
		printf("Now all operation will be saved only in the memory before you confirm to save to the disk file. Input 'help' to show available command.\n");
		printf("Loading root directory...\n\n");
		currentdire.inodecode = 0;
		char currentpath[1024];
		currentpath[0] = '/';
		currentpath[1] = '\0';
		getchar();

	ToReceiveCMD:
		char instruction1[15];
		memset(instruction1, '\0', sizeof(instruction1));
		std::cin.clear();
		std::cin.sync();
		for (unsigned short i = 0; i < 1024; i++)
			if (currentpath[i] != '\0')
				printf("%c", currentpath[i]);
			else
				break;
		printf("> ");
		for (int i = 0;; ++i) {
			char temp = std::cin.get();
			if (temp != ' ' && temp != '/' && temp != '\n')
				instruction1[i] = temp;
			else {
				instruction1[i] = '\0';
				break;
			}
		}
		if (strcmp(instruction1, "createFile") == 0) {
			char createfilepath[1024];
			char createfilename[MAX_FILE_NAME_LENGTH + 1];
			fflush(stdin);
			scanf_s("%s", createfilepath, (unsigned int)sizeof(createfilepath));
			unsigned short createsize;
			scanf_s("%ud", &createsize);
			getchar();
			if (createsize > MAX_FILE_SIZE)
			{
				printf("Size over limit.\n");
				goto ToReceiveCMD;
			}
			else
				cachebuffer.size = createsize*BLOCK_SIZE;
			directory diretocreatefile;
			diretocreatefile.inodecode = currentdire.inodecode;
			readDirectory(&diretocreatefile);
			char dfname[MAX_FILE_NAME_LENGTH + 1];
			unsigned short dfnamei = 0, locre = 0;
			for (unsigned short i = 0; i < 1024; i++)
			{
				if (createfilepath[i] == '/')
				{
					if (i == 0)
					{
						diretocreatefile.inodecode = 0;
						readDirectory(&diretocreatefile);
					}
					else
					{
						if (dfnamei < MAX_FILE_NAME_LENGTH + 1)
							dfname[dfnamei++] = '\0';
						else
						{
							locre = 1;
							break;
						}
						short cuchilddireno = findFile(&diretocreatefile, dfname);
						if (cuchilddireno != FILE_NOT_FOUND && diskins.in[diretocreatefile.filelist[cuchilddireno].inodecode].mode == 1)
						{
							diretocreatefile.inodecode = diretocreatefile.filelist[cuchilddireno].inodecode;
							readDirectory(&diretocreatefile);
						}
						else
						{
							locre = 1;
							break;
						}
					}
					dfnamei = 0;
				}
				else if (createfilepath[i] == '\0')
				{
					if (dfnamei < MAX_FILE_NAME_LENGTH + 1)
						dfname[dfnamei++] = '\0';
					else
						locre = 1;
					if (createfilepath[i - 1] == '/')
						locre = 1;
					else
						strcpy(createfilename, dfname);
					break;
				}
				else
				{
					if (dfnamei < MAX_FILE_NAME_LENGTH)
						dfname[dfnamei++] = createfilepath[i];
					else
					{
						locre = 1;
						break;
					}
				}
			}
			refreshUsedCapatmp();
			refreshFirstAvaInodeP();
			refreshTotalFileSizetmp();
			if (locre == 1)
			{
				printf("Path not found.\n");
				goto ToReceiveCMD;
			}
			srand((unsigned)time(NULL));
			if (cachebuffer.size > 0)
				printf("Generating random string to create the file:\n");
			for (unsigned int i = 0; i < cachebuffer.size; i++)
			{
				cachebuffer.blocks[i / BLOCK_SIZE].byte[i%BLOCK_SIZE] = rand() % 95 + 32; //Random char from (space) to ~
				if (i < 100)
					printf("%c", cachebuffer.blocks[i / BLOCK_SIZE].byte[i%BLOCK_SIZE]);
				else if (i == 100)
					printf("......");
			}
			printf("\n");
			switch (createFile(&diretocreatefile, createfilename, 0))
			{
			case INVALID_DIRE:
				printf("Path not found.\n");
				break;
			case DIRE_FULL:
				printf("The directory is full.\n");
				break;
			case NOT_ENOUGH_SPACE:
				printf("Not enough space available.\n");
				break;
			case ADDRESS_ERROR:
				printf("Address error.\n");
				break;
			case SAME_NAME_FILE_EXIST:
				printf("File with same name already existed.\n");
				break;
			case SIZE_OVER_LIMIT:
				printf("Size over limit.\n");
				break;
			default:
				break;
			}
			refreshUsedCapatmp();
			refreshFirstAvaInodeP();
			refreshTotalFileSizetmp();
			goto ToReceiveCMD;
		}
		else if (strcmp(instruction1, "deleteFile") == 0) {
			char deletefilepath[1024];
			char deletefilename[MAX_FILE_NAME_LENGTH + 1];
			char dfname[MAX_FILE_NAME_LENGTH + 1];
			unsigned short dfnamei = 0, locre = 0;
			directory diretodeletefile;
			diretodeletefile.inodecode = currentdire.inodecode;
			readDirectory(&diretodeletefile);
			fflush(stdin);
			scanf_s("%s", deletefilepath, (unsigned int)sizeof(deletefilepath));
			getchar();
			for (unsigned short i = 0; i < 1024; i++)
			{
				if (deletefilepath[i] == '/')
				{
					if (i == 0)
					{
						diretodeletefile.inodecode = 0;
						readDirectory(&diretodeletefile);
					}
					else
					{
						if (dfnamei < MAX_FILE_NAME_LENGTH + 1)
							dfname[dfnamei++] = '\0';
						else
						{
							locre = 1;
							break;
						}
						short cuchilddireno = findFile(&diretodeletefile, dfname);
						if (cuchilddireno != FILE_NOT_FOUND && diskins.in[diretodeletefile.filelist[cuchilddireno].inodecode].mode == 1)
						{
							diretodeletefile.inodecode = diretodeletefile.filelist[cuchilddireno].inodecode;
							readDirectory(&diretodeletefile);
						}
						else
						{
							locre = 1;
							break;
						}
					}
					dfnamei = 0;
				}
				else if (deletefilepath[i] == '\0')
				{
					if (dfnamei < MAX_FILE_NAME_LENGTH + 1)
						dfname[dfnamei++] = '\0';
					else
						locre = 1;
					if (deletefilepath[i - 1] == '/')
						locre = 1;
					else
						strcpy(deletefilename, dfname);
					break;
				}
				else
				{
					if (dfnamei < MAX_FILE_NAME_LENGTH)
						dfname[dfnamei++] = deletefilepath[i];
					else
					{
						locre = 1;
						break;
					}
				}
			}
			refreshUsedCapatmp();
			refreshFirstAvaInodeP();
			refreshTotalFileSizetmp();
			if (locre == 1)
			{
				printf("Path not found.\n");
				goto ToReceiveCMD;
			}
			switch (deleteFile(&diretodeletefile, deletefilename))
			{
			case FILE_DELETED_FAILED:
				printf("Failed to delete.\n");
				break;
			case FILE_NOT_EXIST:
				printf("Path not found.\n");
				break;
			default:
				break;
			}
			refreshUsedCapatmp();
			refreshFirstAvaInodeP();
			refreshTotalFileSizetmp();
			goto ToReceiveCMD;
		}
		else if (strcmp(instruction1, "createDir") == 0) {
			char createdirepath[1024];
			char createdirename[MAX_FILE_NAME_LENGTH + 1];
			char dfname[MAX_FILE_NAME_LENGTH + 1];
			directory diretocreatedire;
			diretocreatedire.inodecode = currentdire.inodecode;
			readDirectory(&diretocreatedire);
			unsigned short dfnamei = 0, locre = 0;
			fflush(stdin);
			scanf_s("%s", createdirepath, (unsigned int)sizeof(createdirepath));
			getchar();
			for (unsigned short i = 0; i < 1024; i++)
			{
				if (createdirepath[i] == '/')
				{
					if (i == 0)
					{
						diretocreatedire.inodecode = 0;
						readDirectory(&diretocreatedire);
					}
					else
					{
						if (dfnamei < MAX_FILE_NAME_LENGTH + 1)
							dfname[dfnamei++] = '\0';
						else
						{
							locre = 1;
							break;
						}
						short cuchilddireno = findFile(&diretocreatedire, dfname);
						if (cuchilddireno != FILE_NOT_FOUND)
						{
							if (diskins.in[diretocreatedire.filelist[cuchilddireno].inodecode].mode == 1)
							{
								diretocreatedire.inodecode = diretocreatedire.filelist[cuchilddireno].inodecode;
								readDirectory(&diretocreatedire);
							}
							else
							{
								locre = 1;
								break;
							}
						}
						else
						{
							if (createFile(&diretocreatedire, dfname, 1) == FILE_CREATE_SUCCESS)
							{
								readDirectory(&diretocreatedire);
								cuchilddireno = findFile(&diretocreatedire, dfname);
								if (cuchilddireno != FILE_NOT_FOUND)
								{
									diretocreatedire.inodecode = diretocreatedire.filelist[cuchilddireno].inodecode;
									readDirectory(&diretocreatedire);
								}
							}
							else
							{
								locre = 1;
								break;
							}
						}
					}
					dfnamei = 0;
				}
				else if (createdirepath[i] == '\0')
				{
					if (dfnamei < MAX_FILE_NAME_LENGTH + 1)
						dfname[dfnamei++] = '\0';
					else
						locre = 1;
					if (createdirepath[i - 1] == '/')
						locre = 1;
					else
						strcpy(createdirename, dfname);
					break;
				}
				else
				{
					if (dfnamei < MAX_FILE_NAME_LENGTH)
						dfname[dfnamei++] = createdirepath[i];
					else
					{
						locre = 1;
						break;
					}
				}
			}
			refreshUsedCapatmp();
			refreshFirstAvaInodeP();
			refreshTotalFileSizetmp();
			if (locre == 1)
			{
				printf("Path not found.\n");
				goto ToReceiveCMD;
			}
			switch (createFile(&diretocreatedire, createdirename, 1))
			{
			case INVALID_DIRE:
				printf("Path not found.\n");
				break;
			case DIRE_FULL:
				printf("The directory is full.\n");
				break;
			case NOT_ENOUGH_SPACE:
				printf("Not enough space available.\n");
				break;
			case ADDRESS_ERROR:
				printf("Address error.\n");
				break;
			case SAME_NAME_FILE_EXIST:
				printf("File with same name already existed.\n");
				break;
			case SIZE_OVER_LIMIT:
				printf("Size over limit.\n");
				break;
			default:
				break;
			}
			refreshUsedCapatmp();
			refreshFirstAvaInodeP();
			refreshTotalFileSizetmp();
			goto ToReceiveCMD;
		}
		else if (strcmp(instruction1, "deleteDir") == 0) {
			char deletedirepath[1024];
			char deletedirename[MAX_FILE_NAME_LENGTH + 1], dfname[MAX_FILE_NAME_LENGTH + 1];
			unsigned short locre = 0, dfnamei = 0;
			directory diretodeletedire;
			diretodeletedire.inodecode = currentdire.inodecode;
			readDirectory(&diretodeletedire);
			fflush(stdin);
			scanf_s("%s", deletedirepath, (unsigned int)sizeof(deletedirepath));
			getchar();
			for (unsigned short i = 0; i < 1024; i++)
			{
				if (deletedirepath[i] == '/')
				{
					if (i == 0)
					{
						diretodeletedire.inodecode = 0;
						readDirectory(&diretodeletedire);
					}
					else
					{
						if (dfnamei < MAX_FILE_NAME_LENGTH + 1)
							dfname[dfnamei++] = '\0';
						else
						{
							locre = 1;
							break;
						}
						short cuchilddireno = findFile(&diretodeletedire, dfname);
						if (cuchilddireno != FILE_NOT_FOUND && diskins.in[diretodeletedire.filelist[cuchilddireno].inodecode].mode == 1)
						{
							diretodeletedire.inodecode = diretodeletedire.filelist[cuchilddireno].inodecode;
							readDirectory(&diretodeletedire);
						}
						else
						{
							locre = 1;
							break;
						}
					}
					dfnamei = 0;
				}
				else if (deletedirepath[i] == '\0')
				{
					if (dfnamei < MAX_FILE_NAME_LENGTH + 1)
						dfname[dfnamei++] = '\0';
					else
						locre = 1;
					if (deletedirepath[i - 1] == '/')
						locre = 1;
					else
						strcpy(deletedirename, dfname);
					break;
				}
				else
				{
					if (dfnamei < MAX_FILE_NAME_LENGTH)
						dfname[dfnamei++] = deletedirepath[i];
					else
					{
						locre = 1;
						break;
					}
				}
			}
			refreshUsedCapatmp();
			refreshFirstAvaInodeP();
			refreshTotalFileSizetmp();
			if (locre == 1)
			{
				printf("Path not found.\n");
				goto ToReceiveCMD;
			}
			short targetDireno = findFile(&diretodeletedire, deletedirename);
			if (targetDireno != FILE_NOT_FOUND && diretodeletedire.filelist[targetDireno].inodecode == currentdire.inodecode)
			{
				printf("Could not delete current working directory.\n");
				goto ToReceiveCMD;
			}
			switch (deleteDire(&diretodeletedire, deletedirename))
			{
			case FILE_DELETED_FAILED:
				printf("Failed to delete.\n");
				break;
			case FILE_NOT_EXIST:
				printf("Path not found.\n");
				break;
			default:
				break;
			}
			refreshUsedCapatmp();
			refreshFirstAvaInodeP();
			refreshTotalFileSizetmp();
			goto ToReceiveCMD;
		}
		else if (strcmp(instruction1, "changeDir") == 0) {
			char targetdirepath[1024];
			fflush(stdin);
			scanf_s("%s", targetdirepath, (unsigned int)sizeof(targetdirepath));
			getchar();
			if (strcmp(targetdirepath, ".") == 0)
				goto ToReceiveCMD;
			else if (strcmp(targetdirepath, "..") == 0)
			{
				if (currentdire.inodecode != 0)
					for (unsigned short i = 0; i<1024; i++)
						if (currentpath[i] == '\0' || i == 1023)
						{
							for (unsigned short j = i; j >= 0; j--)
								if (currentpath[j] == '/')
								{
									currentpath[j] = '\0';
									break;
								}
							break;
						}
				currentdire.inodecode = diskins.in[currentdire.inodecode].parentcode;
				if (currentdire.inodecode == 0)
				{
					currentpath[0] = '/';
					currentpath[1] = '\0';
				}
				goto ToReceiveCMD;
			}
			directory targetdire;
			targetdire.inodecode = currentdire.inodecode;
			unsigned short dfnamei = 0, locre = 0, resetcurrpath = 0;
			char dfname[MAX_FILE_NAME_LENGTH + 1];
			for (unsigned short i = 0; i < 1024; i++)
			{
				if (targetdirepath[i] == '/')
				{
					if (i == 0)
					{
						targetdire.inodecode = 0;
						readDirectory(&targetdire);
						resetcurrpath = 1;
					}
					else
					{
						if (dfnamei < MAX_FILE_NAME_LENGTH + 1)
							dfname[dfnamei++] = '\0';
						else
						{
							locre = 1;
							break;
						}
						short cuchilddireno = findFile(&targetdire, dfname);
						if (cuchilddireno != FILE_NOT_FOUND && diskins.in[targetdire.filelist[cuchilddireno].inodecode].mode == 1)
						{
							targetdire.inodecode = targetdire.filelist[cuchilddireno].inodecode;
							readDirectory(&targetdire);
						}
						else
						{
							locre = 1;
							break;
						}
					}
					dfnamei = 0;
				}
				else if (targetdirepath[i] == '\0')
				{
					if (targetdirepath[i - 1] == '/')
					{
						if (i > 1)
							targetdirepath[i - 1] = '\0';
						break;
					}
					if (dfnamei < MAX_FILE_NAME_LENGTH + 1)
					{
						dfname[dfnamei++] = '\0';
						short cuchilddireno = findFile(&targetdire, dfname);
						if (cuchilddireno != FILE_NOT_FOUND && diskins.in[targetdire.filelist[cuchilddireno].inodecode].mode == 1)
						{
							targetdire.inodecode = targetdire.filelist[cuchilddireno].inodecode;
							readDirectory(&targetdire);
						}
						else
							locre = 1;
						break;
					}
					else
					{
						locre = 1;
						break;
					}
				}
				else
				{
					if (dfnamei < MAX_FILE_NAME_LENGTH)
						dfname[dfnamei++] = targetdirepath[i];
					else
					{
						locre = 1;
						break;
					}
				}
			}
			if (locre == 1)
			{
				printf("Path not found.\n");
				goto ToReceiveCMD;
			}
			if (resetcurrpath == 1)
				strcpy(currentpath, targetdirepath);
			else
				if (currentdire.inodecode == 0)
					strcat(currentpath, targetdirepath);
				else
				{
					strcat(currentpath, "/");
					strcat(currentpath, targetdirepath);
				}
			currentdire.inodecode = targetdire.inodecode;
			goto ToReceiveCMD;
		}
		else if (strcmp(instruction1, "dir") == 0) {
			printDirectory(&currentdire);
			goto ToReceiveCMD;
		}
		else if (strcmp(instruction1, "cp") == 0) {
			char sourcefilepath[1024], targetfilepath[1024];
			char sourcefilename[MAX_FILE_NAME_LENGTH + 1], targetfilename[MAX_FILE_NAME_LENGTH + 1];
			fflush(stdin);
			scanf_s("%s", sourcefilepath, (unsigned int)sizeof(sourcefilepath));
			scanf_s("%s", targetfilepath, (unsigned int)sizeof(targetfilepath));
			getchar();
			directory direofsourcefile, direoftargetfile;
			direofsourcefile.inodecode = currentdire.inodecode;
			readDirectory(&direofsourcefile);
			direoftargetfile.inodecode = currentdire.inodecode;
			readDirectory(&direoftargetfile);
			char dfname[MAX_FILE_NAME_LENGTH + 1];
			unsigned short dfnamei = 0, locre = 0;
			for (unsigned short i = 0; i < 1024; i++)
			{
				if (sourcefilepath[i] == '/')
				{
					if (i == 0)
					{
						direofsourcefile.inodecode = 0;
						readDirectory(&direofsourcefile);
					}
					else
					{
						if (dfnamei < MAX_FILE_NAME_LENGTH + 1)
							dfname[dfnamei++] = '\0';
						else
						{
							locre = 1;
							break;
						}
						short cuchilddireno = findFile(&direofsourcefile, dfname);
						if (cuchilddireno != FILE_NOT_FOUND && diskins.in[direofsourcefile.filelist[cuchilddireno].inodecode].mode == 1)
						{
							direofsourcefile.inodecode = direofsourcefile.filelist[cuchilddireno].inodecode;
							readDirectory(&direofsourcefile);
						}
						else
						{
							locre = 1;
							break;
						}
					}
					dfnamei = 0;
				}
				else if (sourcefilepath[i] == '\0')
				{
					if (dfnamei < MAX_FILE_NAME_LENGTH + 1)
						dfname[dfnamei++] = '\0';
					else
						locre = 1;
					if (sourcefilepath[i - 1] == '/')
						locre = 1;
					else
						strcpy(sourcefilename, dfname);
					break;
				}
				else
				{
					if (dfnamei < MAX_FILE_NAME_LENGTH)
						dfname[dfnamei++] = sourcefilepath[i];
					else
					{
						locre = 1;
						break;
					}
				}
			}
			if (locre == 1)
			{
				printf("Path not found.\n");
				goto ToReceiveCMD;
			}
			locre = 0;
			dfnamei = 0;
			for (unsigned short i = 0; i < 1024; i++)
			{
				if (targetfilepath[i] == '/')
				{
					if (i == 0)
					{
						direoftargetfile.inodecode = 0;
						readDirectory(&direoftargetfile);
					}
					else
					{
						if (dfnamei < MAX_FILE_NAME_LENGTH + 1)
							dfname[dfnamei++] = '\0';
						else
						{
							locre = 1;
							break;
						}
						short cuchilddireno = findFile(&direoftargetfile, dfname);
						if (cuchilddireno != FILE_NOT_FOUND && diskins.in[direoftargetfile.filelist[cuchilddireno].inodecode].mode == 1)
						{
							direoftargetfile.inodecode = direoftargetfile.filelist[cuchilddireno].inodecode;
							readDirectory(&direoftargetfile);
						}
						else
						{
							locre = 1;
							break;
						}
					}
					dfnamei = 0;
				}
				else if (targetfilepath[i] == '\0')
				{
					if (dfnamei < MAX_FILE_NAME_LENGTH + 1)
						dfname[dfnamei++] = '\0';
					else
						locre = 1;
					if (targetfilepath[i - 1] == '/')
						locre = 1;
					else
						strcpy(targetfilename, dfname);
					break;
				}
				else
				{
					if (dfnamei < MAX_FILE_NAME_LENGTH)
						dfname[dfnamei++] = targetfilepath[i];
					else
					{
						locre = 1;
						break;
					}
				}
			}
			if (locre == 1)
			{
				printf("Path not found.\n");
				goto ToReceiveCMD;
			}
			short sourcefileno = findFile(&direofsourcefile, sourcefilename);
			if (sourcefileno != FILE_NOT_FOUND)
			{
				file sourcefile;
				sourcefile.inodecode = direofsourcefile.filelist[sourcefileno].inodecode;
				cachebuffer.size = diskins.in[sourcefile.inodecode].fileSize;
				unsigned short adderr = 0;
				for (unsigned int i = 0; i < cachebuffer.size; i += BLOCK_SIZE)
				{
					blockbyteaddress* bbac = fileAddr2StorageAddr(&sourcefile, i);
					if (bbac != nullptr)
						for (bbac->byteaddr = 0; bbac->byteaddr < ((cachebuffer.size - i) > BLOCK_SIZE ? BLOCK_SIZE : (cachebuffer.size - i)); bbac->byteaddr++)
							cachebuffer.blocks[i / BLOCK_SIZE].byte[bbac->byteaddr] = diskstor.blocks[bbac->blockaddr].byte[bbac->byteaddr];
					else
					{
						printf("Address error.\n");
						adderr = 1;
						break;
					}
				}
				if (adderr == 1)
				{
					refreshUsedCapatmp();
					refreshFirstAvaInodeP();
					refreshTotalFileSizetmp();
					goto ToReceiveCMD;
				}
				switch (createFile(&direoftargetfile, targetfilename, 0))
				{
				case INVALID_DIRE:
					printf("Path not found.\n");
					break;
				case DIRE_FULL:
					printf("The directory is full.\n");
					break;
				case NOT_ENOUGH_SPACE:
					printf("Not enough space available.\n");
					break;
				case ADDRESS_ERROR:
					printf("Address error.\n");
					break;
				case SAME_NAME_FILE_EXIST:
					printf("File with same name already existed.\n");
					break;
				case SIZE_OVER_LIMIT:
					printf("Size over limit.\n");
					break;
				default:
					break;
				}
				refreshUsedCapatmp();
				refreshFirstAvaInodeP();
				refreshTotalFileSizetmp();
				goto ToReceiveCMD;
			}
			else
			{
				printf("Path not found.\n");
				goto ToReceiveCMD;
			}
		}
		else if (strcmp(instruction1, "sum") == 0) {
			printf("Status of virtual disk in memory:\n");
			printSuperBlock(sbtmp);
			printf("Status of virtual disk file:\n");
			printSuperBlock(sb);
			goto ToReceiveCMD;
		}
		else if (strcmp(instruction1, "cat") == 0) {
			char printfilepath[1024];
			char printfilename[MAX_FILE_NAME_LENGTH + 1], dfname[MAX_FILE_NAME_LENGTH + 1];
			unsigned short dfnamei = 0, locre = 0;
			scanf_s("%s", printfilepath, (unsigned int)sizeof(printfilepath));
			getchar();
			directory diretoprintfile;
			diretoprintfile.inodecode = currentdire.inodecode;
			readDirectory(&diretoprintfile);
			for (unsigned short i = 0; i < 1024; i++)
			{
				if (printfilepath[i] == '/')
				{
					if (i == 0)
					{
						diretoprintfile.inodecode = 0;
						readDirectory(&diretoprintfile);
					}
					else
					{
						if (dfnamei < MAX_FILE_NAME_LENGTH + 1)
							dfname[dfnamei++] = '\0';
						else
						{
							locre = 1;
							break;
						}
						short cuchilddireno = findFile(&diretoprintfile, dfname);
						if (cuchilddireno != FILE_NOT_FOUND && diskins.in[diretoprintfile.filelist[cuchilddireno].inodecode].mode == 1)
						{
							diretoprintfile.inodecode = diretoprintfile.filelist[cuchilddireno].inodecode;
							readDirectory(&diretoprintfile);
						}
						else
						{
							locre = 1;
							break;
						}
					}
					dfnamei = 0;
				}
				else if (printfilepath[i] == '\0')
				{
					if (dfnamei < MAX_FILE_NAME_LENGTH + 1)
						dfname[dfnamei++] = '\0';
					else
						locre = 1;
					if (printfilepath[i - 1] == '/')
						locre = 1;
					else
						strcpy(printfilename, dfname);
					break;
				}
				else
				{
					if (dfnamei < MAX_FILE_NAME_LENGTH)
						dfname[dfnamei++] = printfilepath[i];
					else
					{
						locre = 1;
						break;
					}
				}
			}
			if (locre == 1)
			{
				printf("Path not found.\n");
				goto ToReceiveCMD;
			}
			short printfileno = findFile(&diretoprintfile, printfilename);
			if (printfileno != FILE_NOT_FOUND)
			{
				file printfile;
				printfile.inodecode = diretoprintfile.filelist[printfileno].inodecode;
				printFile(&printfile);
				printf("\n");
			}
			else
				printf("Path not found!\n");
			goto ToReceiveCMD;
		}
		else if (strcmp(instruction1, "save") == 0) {
			printf("Writing super block...\n");
			fseek(disk, BOOT_BLOCK_SIZE*BLOCK_SIZE, SEEK_SET);
			if (fwrite(&sbtmp, sizeof(superblock), 1, disk) != 1)
			{
				printf("Failed to write super block!\n");
				fclose(disk);
				return WRITE_DISK_FILE_FAILED;
			}
			printf("Writing bitmap...\n");
			fseek(disk, (BOOT_BLOCK_SIZE + SUPER_BLOCK_SIZE)*BLOCK_SIZE, SEEK_SET);
			if (fwrite(&diskbm, sizeof(bitmap), 1, disk) != 1)
			{
				printf("Failed to write bitmap!\n");
				fclose(disk);
				return WRITE_DISK_FILE_FAILED;
			}
			printf("Writing i-nodes...\n");
			fseek(disk, (BOOT_BLOCK_SIZE + SUPER_BLOCK_SIZE)*BLOCK_SIZE + sizeof(bitmap), SEEK_SET);
			if (fwrite(&diskins, sizeof(inodes), 1, disk) != 1)
			{
				printf("Failed to write i-nodes!\n");
				fclose(disk);
				return WRITE_DISK_FILE_FAILED;
			}
			printf("Writing storage...\n");
			fseek(disk, (BOOT_BLOCK_SIZE + SUPER_BLOCK_SIZE)*BLOCK_SIZE + sizeof(bitmap) + sizeof(inodes), SEEK_SET);
			if (fwrite(&diskstor, sizeof(storage), 1, disk) != 1)
			{
				printf("Failed to write storage!\n");
				fclose(disk);
				return WRITE_DISK_FILE_FAILED;
			}
			printf("Reading super block of virtual disk file...\n");
			fseek(disk, BOOT_BLOCK_SIZE*BLOCK_SIZE, SEEK_SET);
			if (fread(&sb, sizeof(superblock), 1, disk) != 1)
			{
				printf("Read virtual disk failed!\n");
				fclose(disk);
				return READ_DISK_FILE_FAILED;
			}
			printf("Finished.\n");
			goto ToReceiveCMD;
		}
		else if (strcmp(instruction1, "exit") == 0) {
			printf("Status of virtual disk in memory:\n");
			printSuperBlock(sbtmp);
			printf("Status of virtual disk file:\n");
			printSuperBlock(sb);
			char tosave;
		CHOICE:
			printf("Save changes to virtual disk file?[y/n]\n");
			scanf("%c", &tosave);
			getchar();
			if (tosave == 'y')
			{
				printf("Writing super block...\n");
				fseek(disk, BOOT_BLOCK_SIZE*BLOCK_SIZE, SEEK_SET);
				if (fwrite(&sbtmp, sizeof(superblock), 1, disk) != 1)
				{
					printf("Failed to write super block!\n");
					fclose(disk);
					return WRITE_DISK_FILE_FAILED;
				}
				printf("Writing bitmap...\n");
				fseek(disk, (BOOT_BLOCK_SIZE + SUPER_BLOCK_SIZE)*BLOCK_SIZE, SEEK_SET);
				if (fwrite(&diskbm, sizeof(bitmap), 1, disk) != 1)
				{
					printf("Failed to write bitmap!\n");
					fclose(disk);
					return WRITE_DISK_FILE_FAILED;
				}
				printf("Writing i-nodes...\n");
				fseek(disk, (BOOT_BLOCK_SIZE + SUPER_BLOCK_SIZE)*BLOCK_SIZE + sizeof(bitmap), SEEK_SET);
				if (fwrite(&diskins, sizeof(inodes), 1, disk) != 1)
				{
					printf("Failed to write i-nodes!\n");
					fclose(disk);
					return WRITE_DISK_FILE_FAILED;
				}
				printf("Writing storage...\n");
				fseek(disk, (BOOT_BLOCK_SIZE + SUPER_BLOCK_SIZE)*BLOCK_SIZE + sizeof(bitmap) + sizeof(inodes), SEEK_SET);
				if (fwrite(&diskstor, sizeof(storage), 1, disk) != 1)
				{
					printf("Failed to write storage!\n");
					fclose(disk);
					return WRITE_DISK_FILE_FAILED;
				}
				printf("Reading super block of virtual disk file...\n");
				fseek(disk, BOOT_BLOCK_SIZE*BLOCK_SIZE, SEEK_SET);
				if (fread(&sb, sizeof(superblock), 1, disk) != 1)
				{
					printf("Read virtual disk failed!\n");
					fclose(disk);
					return READ_DISK_FILE_FAILED;
				}
				printf("Finished.\n");
			}
			else if (tosave == 'n')
				printf("Give up saving.\n");
			else
				goto CHOICE;
		}
		else if (strcmp(instruction1, "help") == 0) {
			printf("Create a file：createFile fileName fileSize\ti.e.：createFile /dir1/myFile 10 (in KB)\n");
			printf("Delete a file：deleteFile filename\ti.e.：deleteFile /dir1/myFile\n");
			printf("Create a directory：createDir\ti.e.：createDir /dir1/sub1\n");
			printf("Delete a directory：deleteDir\ti.e.: deleteDir /dir1/sub1\n");
			printf("Change current working direcotry：changeDir\ti.e.: changeDir /dir2\n");
			printf("List all the files and sub-directories under current working directory：dir\n");
			printf("Copy a file : cp\ti.e.: cp file1 file2\n");
			printf("Display the usage of storage space：sum\n");
			printf("Print out the file contents: cat\ti.e:  cat /dir1/file1\n");
			printf("Save to virtual disk file: save\n");
			printf("Close the disk and exit: exit\n");
			goto ToReceiveCMD;
		}
		else
			goto ToReceiveCMD;

		fclose(disk);
		return TO_BACK;
	}
	else if (choice == 2)
	{
		fclose(disk);
		printf("Confirm to format? Input 11 to continue, other integers to give up:\n");
		scanf("%hd", &choice);
		if (choice == 11)
			return TO_FORMAT;
		else
			return TO_BACK;
	}
	else if (choice == 99)
	{
		isdebug = true;
		printf("Reading bitmap and loading it in memory...\n");
		fseek(disk, (BOOT_BLOCK_SIZE + SUPER_BLOCK_SIZE)*BLOCK_SIZE, SEEK_SET);
		if (fread(&diskbm, sizeof(bitmap), 1, disk) != 1)
		{
			printf("Read bitmap failed!\n\n");
			fclose(disk);
			return READ_DISK_FILE_FAILED;
		}
		refreshAvaBlockP();


		printf("Reading i-nodes and loading it in memory...\n");
		fseek(disk, (BOOT_BLOCK_SIZE + SUPER_BLOCK_SIZE)*BLOCK_SIZE + sizeof(bitmap), SEEK_SET);
		if (fread(&diskins, sizeof(inodes), 1, disk) != 1)
		{
			printf("Read inodes failed!\n\n");
			fclose(disk);
			return READ_DISK_FILE_FAILED;
		}
		refreshFirstAvaInodeP();
		printf("First available inode code: %u.\n", firstAvaInodeP);


		printf("%ld\n", ftell(disk));
		printf("Reading storage and loading it in memory...\n");
		fseek(disk, (BOOT_BLOCK_SIZE + SUPER_BLOCK_SIZE)*BLOCK_SIZE + sizeof(bitmap) + sizeof(inodes), SEEK_SET);
		if (fread(&diskstor, sizeof(storage), 1, disk) != 1)
		{
			printf("Read storage failed!\n\n");
			fclose(disk);
			return READ_DISK_FILE_FAILED;
		}


		printf("%ld\n", ftell(disk));
		sbtmp.superBlockSize = sb.superBlockSize;
		sbtmp.inodeNumDiv16_1 = sb.inodeNumDiv16_1;
		sbtmp.diskSize = sb.diskSize;
		sbtmp.capacity = sb.capacity;
		sbtmp.usedcapa = sb.usedcapa;
		sbtmp.createTime = sb.createTime;
		sbtmp.avainode = sb.avainode;
		sbtmp.bmstoragestartpoint = sb.bmstoragestartpoint;
		sbtmp.totalfilesize = sb.totalfilesize;
		sbtmp.lastWriteTime = sb.lastWriteTime;
		refreshUsedCapatmp();
		refreshTotalFileSizetmp();
		printf("Now all operation will be saved only in the memory before you confirm to save to the disk file.\n");
		printf("Loading root directory...\n\n");
		currentdire.inodecode = 0;
		char currentpath[1024];
		currentpath[0] = '/';
		currentpath[1] = '\0';
		bool redebug = true;
		while (redebug)
		{
		char debugcmd;
		printf("Current path: %s\n", currentpath);
		printDirectory(&currentdire);
			printf("\nDebug Mode in current directory:  a: go to parent directory  b: create a file  c: delete a file  d: create a directory  e: delete a directory  g: go to a child directory  h: copy a file to parent directory  i: display usage  j: print a file  s: save changes to disk file  x: exit\n");
			getchar();
			fflush(stdin);
			scanf("%c", &debugcmd);
			switch (debugcmd)
			{
			case 'a':
				if (currentdire.inodecode!=0)
					for (unsigned short i=0;i<1024;i++)
						if (currentpath[i] == '\0' || i == 1023)
						{
							for (unsigned short j=i;j>=0;j--)
								if (currentpath[j] == '/')
								{
									currentpath[j] = '\0';
									break;
								}
							break;
						}
				currentdire.inodecode = diskins.in[currentdire.inodecode].parentcode;
				if (currentdire.inodecode == 0)
				{
					currentpath[0] = '/';
					currentpath[1] = '\0';
				}
				break;
			case 'b':
				char createfilename[MAX_FILE_NAME_LENGTH + 1];
				fflush(stdin);
				scanf("%s", createfilename);
				scanf("%ud", &(cachebuffer.size));
				srand((unsigned)time(NULL));
				if (cachebuffer.size > 0)
					printf("Generating random string to create the file:\n");
				for (unsigned int i = 0; i < cachebuffer.size; i++)
				{
					cachebuffer.blocks[i / BLOCK_SIZE].byte[i%BLOCK_SIZE] = rand() % 95 + 32; //Random char from (space) to ~
					if (i < 100)
						printf("%c", cachebuffer.blocks[i / BLOCK_SIZE].byte[i%BLOCK_SIZE]);
					else if (i == 100)
						printf("......");
				}
				printf("\nResult: %d.\n", createFile(&currentdire, createfilename, 0));
				refreshUsedCapatmp();
				refreshTotalFileSizetmp();
				break;
			case 'c':
				char deletefilename[MAX_FILE_NAME_LENGTH + 1];
				fflush(stdin);
				scanf("%s", deletefilename);
				printf("Result: %d.\n", deleteFile(&currentdire, deletefilename));
				refreshUsedCapatmp();
				refreshTotalFileSizetmp();
				break;
			case 'd':
				char createdirename[MAX_FILE_NAME_LENGTH + 1];
				fflush(stdin);
				scanf("%s", createdirename);
				printf("Result: %d.\n", createFile(&currentdire, createdirename, 1));
				refreshUsedCapatmp();
				refreshTotalFileSizetmp();
				break;
			case 'e':
				char deletedirename[MAX_FILE_NAME_LENGTH + 1];
				fflush(stdin);
				scanf("%s", deletedirename);
				printf("Result: %d.\n", deleteDire(&currentdire, deletedirename));
				refreshUsedCapatmp();
				refreshTotalFileSizetmp();
				break;
			case 'g':
			{
				char childdirename[MAX_FILE_NAME_LENGTH + 1];
				fflush(stdin);
				scanf("%s", childdirename);
				short childdireno = findFile(&currentdire, childdirename);
				if (childdireno == FILE_NOT_FOUND)
					printf("Directory not found!\n");
				else if (diskins.in[currentdire.filelist[childdireno].inodecode].mode != 1)
					printf("Not a directory!\n");
				else
				{
					if (currentdire.inodecode == 0)
					{
						currentpath[0] = '/';
						unsigned short j = 0;
						do
							currentpath[j + 1] = currentdire.filelist[childdireno].filename[j];
						while (currentdire.filelist[childdireno].filename[j++] != '\0' && j < MAX_FILE_NAME_LENGTH);
						if (currentdire.filelist[childdireno].filename[j - 1] != '\0')
							currentpath[j + 1] = '\0';
					}
					else
						for (unsigned short i = 0; i < 1024; i++)
							if (currentpath[i] == '\0')
							{
								currentpath[i++] = '/';
								unsigned short j = 0;
								do
									currentpath[i + j] = currentdire.filelist[childdireno].filename[j];
								while (currentdire.filelist[childdireno].filename[j++] != '\0' && j < MAX_FILE_NAME_LENGTH);
								if (currentdire.filelist[childdireno].filename[j - 1] != '\0')
									currentpath[i + j] = '\0';
								break;
							}
					currentdire.inodecode = currentdire.filelist[childdireno].inodecode;
				}
			}
				break;
			case 'h':
			{
				char copyfilename[MAX_FILE_NAME_LENGTH + 1];
				fflush(stdin);
				scanf("%s", copyfilename);
				short copyfileno = findFile(&currentdire, copyfilename);
				if (copyfileno == FILE_NOT_FOUND)
					printf("File not found!\n");
				else if (diskins.in[currentdire.filelist[copyfileno].inodecode].mode == 0)
				{
					if (diskins.in[diskins.in[currentdire.inodecode].parentcode].fileSize / sizeof(fileentry) >= MAX_FILE_NUM_IN_DIRE)
						printf("Parent directory full!\n");
					else
					{
						bool adderr = false;
						cachebuffer.size = diskins.in[currentdire.filelist[copyfileno].inodecode].fileSize;
						file filetocopy;
						filetocopy.inodecode = currentdire.filelist[copyfileno].inodecode;
						for (unsigned int i = 0; i < cachebuffer.size; i+=BLOCK_SIZE)
						{
							blockbyteaddress* bbac = fileAddr2StorageAddr(&filetocopy, i);
							if (bbac != nullptr)
								for (bbac->byteaddr = 0; bbac->byteaddr < ((cachebuffer.size - i) > BLOCK_SIZE ? BLOCK_SIZE : (cachebuffer.size - i)); bbac->byteaddr++)
									cachebuffer.blocks[i / BLOCK_SIZE].byte[bbac->byteaddr] = diskstor.blocks[bbac->blockaddr].byte[bbac->byteaddr];
							else
							{
								adderr = true;
								printf("Address error!\n");
							}
						}
						if (!adderr)
						{
							directory parentdirectory;
							parentdirectory.inodecode = diskins.in[currentdire.inodecode].parentcode;
							printf("Result: %d.\n", createFile(&parentdirectory, copyfilename, 0));
							refreshTotalFileSizetmp();
							refreshUsedCapatmp();
						}
					}
				}
				else
					printf("It is a directory!\n");
			}
				break;
			case 'i':
				printf("Status of virtual disk in memory:\n");
				printSuperBlock(sbtmp);
				printf("Status of virtual disk file:\n");
				printSuperBlock(sb);
				break;
			case 'j':
			{
				char printfilename[MAX_FILE_NAME_LENGTH + 1];
				fflush(stdin);
				scanf("%s", printfilename);
				short printfileno = findFile(&currentdire, printfilename);
				if (printfileno != FILE_NOT_FOUND)
				{
					file printfile;
					printfile.inodecode = currentdire.filelist[printfileno].inodecode;
					printFile(&printfile);
					printf("\n");
				}
				else
					printf("File not found!\n");
			}
				break;
			case 's':
				printf("Writing super block...\n");
				fseek(disk, BOOT_BLOCK_SIZE*BLOCK_SIZE, SEEK_SET);
				if (fwrite(&sbtmp, sizeof(superblock), 1, disk) != 1)
				{
					printf("Failed to write super block!\n");
					fclose(disk);
					return WRITE_DISK_FILE_FAILED;
				}
				printf("Writing bitmap...\n");
				fseek(disk, (BOOT_BLOCK_SIZE + SUPER_BLOCK_SIZE)*BLOCK_SIZE, SEEK_SET);
				if (fwrite(&diskbm, sizeof(bitmap), 1, disk) != 1)
				{
					printf("Failed to write bitmap!\n");
					fclose(disk);
					return WRITE_DISK_FILE_FAILED;
				}
				printf("Writing i-nodes...\n");
				fseek(disk, (BOOT_BLOCK_SIZE + SUPER_BLOCK_SIZE)*BLOCK_SIZE + sizeof(bitmap), SEEK_SET);
				if (fwrite(&diskins, sizeof(inodes), 1, disk) != 1)
				{
					printf("Failed to write i-nodes!\n");
					fclose(disk);
					return WRITE_DISK_FILE_FAILED;
				}
				printf("Writing storage...\n");
				fseek(disk, (BOOT_BLOCK_SIZE + SUPER_BLOCK_SIZE)*BLOCK_SIZE + sizeof(bitmap) + sizeof(inodes), SEEK_SET);
				if (fwrite(&diskstor, sizeof(storage), 1, disk) != 1)
				{
					printf("Failed to write storage!\n");
					fclose(disk);
					return WRITE_DISK_FILE_FAILED;
				}
				printf("Reading super block of virtual disk file...\n");
				fseek(disk, BOOT_BLOCK_SIZE*BLOCK_SIZE, SEEK_SET);
				if (fread(&sb, sizeof(superblock), 1, disk) != 1)
				{
					printf("Read virtual disk failed!\n");
					fclose(disk);
					return READ_DISK_FILE_FAILED;
				}
				printf("Finished.\n");
				break;
			case 'x':
				redebug = false;
				break;
			default:
				break;
			}
		}
		fclose(disk);
		return TO_BACK;
	}
	else
	{
		fclose(disk);
		return TO_BACK;
	}
}