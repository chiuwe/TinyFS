#include "libDisk.h"
#include "TinyFS_errno.h"

// all block metadata
#define BLOCKTYPE 0
#define MAGICBYTE 1
#define NEXTBYTE 2

// superblock metadata
#define TFSSIZE 3
#define FREELIST 4
#define FREECOUNT 5
#define INODEBYTE 6

// extent metadata
#define TEOF 3

// block type value
#define SUPERBLOCK 1
#define INODE 2
#define EXTENT 3
#define FREE 4

// magic number
#define MAGICNUMBER 0x45

// misc.
#define METADATA 6
#define MAXFILELENGTH 8
#define INODESLOT 10
#define MAXINODE 25

// text color
#define RESET "\033[0m"
#define RED "\e[31m"

/* The default size of the disk and file system block */
#define BLOCKSIZE 256
/* Your program should use a 10240 Byte disk size giving you 40 blocks total. This is a default size. You must be able to support different possible values */
#define DEFAULT_DISK_SIZE 10240
/* use this name for a default disk file name */
#define DEFAULT_DISK_NAME “tinyFSDisk”
typedef int fileDescriptor;

#ifndef LIB_TINY_FS
#define LIB_TINY_FS

int tfs_mkfs(char *filename, int nBytes);
int tfs_mount(char *filename);
int tfs_unmount(void);
fileDescriptor tfs_openFile(char *name);
int tfs_closeFile(fileDescriptor FD);
int tfs_writeFile(fileDescriptor FD,char *buffer, int size);
int tfs_deleteFile(fileDescriptor FD);
int tfs_readByte(fileDescriptor FD, char *buffer);
int tfs_seek(fileDescriptor FD, int offset);
int tfs_rename(char* old, char* new);
int tfs_readdir();
int tfs_makeRO(char* name);
int tfs_makeRW(char* name);
int tfs_writeByte(fileDescriptor FD, char data);
int tfs_createDir(char* dirName);
int tfs_removeDir(char* dirName);
int tfs_removeAll(char* dirName);

#endif