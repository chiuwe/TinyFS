#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "libDisk.h"

// max byte size.

/* This functions opens a regular UNIX file and designates the first nBytes of it as space for the emulated disk. nBytes should be an integral number of the block size. If nBytes > 0 and there is already a file by the given filename, that file’s content may be overwritten. If nBytes is 0, an existing disk is opened, and should not be overwritten. There is no requirement to maintain integrity of any file content beyond nBytes. The return value is -1 on failure or a disk number on success. */
int openDisk(char *filename, int nBytes) {
   int fd;
   
   if (nBytes % BLOCKSIZE) {
      return -1;
   }
   
   fd = open(filename, O_CREAT|O_RDWR, S_IRWXU);
   if (fd == -1) {
      return -1;
   }
   
   if (nBytes) {
      if (lseek(fd, nBytes, SEEK_SET) == -1) {
         return -1;
      }
      
      if (write(fd, "", 1) < 0) {
         return -1;
      }
   }
   
   return fd;
}

/* readBlock() reads an entire block of blockSize bytes from the open disk (identified by ‘disk’) and copies the result into a local buffer (must be at least of blockSize bytes). The bNum is a logical block number, which must be translated into a byte offset within the disk. The translation from logical to physical block is straightforward: bNum=0 is the very first byte of the file. bNum=1 is blockSize bytes into the disk, bNum=n is n*blockSize bytes into the disk. On success, it returns 0. ­-1 or smaller is returned if disk is not available (hasn’t been opened) or any other failures. You must define your own error code system. */
int readBlock(int disk, int bNum, void *block) {
   
   // check if past EOF.
   if (bNum * BLOCKSIZE > lseek(disk, 0, SEEK_END)) {
      return -1;
   }
   if (lseek(disk, BLOCKSIZE * bNum, SEEK_SET) == -1) {
      return -1;
   }
   if (read(disk, block, BLOCKSIZE) == -1) {
      return -1;
   }
   
   return 0;
}

/* writeBlock() takes disk number ‘disk’ and logical block number ‘bNum’ and writes the content of the buffer ‘block’ to that location. ‘block’ must be integral with blockSize. Just as in readBlock(), writeBlock() must translate the logical block bNum to the correct byte position in the file. On success, it returns 0. -­1 or smaller is returned if disk is not available (i.e. hasn’t been opened) or any other failures. You must define your own error code system. */
int writeBlock(int disk, int bNum, void *block) {
   
   // check if past EOF.
   if (bNum * BLOCKSIZE > lseek(disk, 0, SEEK_END)) {
      return -1;
   }
   if (lseek(disk, BLOCKSIZE * bNum, SEEK_SET) == -1) {
      return -1;
   }
   if (write(disk, block, BLOCKSIZE) == -1) {
      return -1;
   }
   
   return 0;
}
