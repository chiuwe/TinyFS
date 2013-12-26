#define BLOCKSIZE 256

#ifndef LIB_DISK
#define LIB_DISK

int openDisk(char *filename, int nBytes);
int readBlock(int disk, int bNum, void *block);
int writeBlock(int disk, int bNum, void *block);

#endif