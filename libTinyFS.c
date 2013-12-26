#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <math.h>
#include "libTinyFS.h"

// improve deleteInode/deleteFile, reorganize inodeslot, pulling last one, freeing blocks if needed. if implemented change removeDir to check for nextbyte first, fix print as well remove first if in first while.

typedef struct{
   int block;
   int bp;
   int fp;
} entry;

entry *table;
int tsize;
int tcount = 0;
int mounted = 0;
int previous;
char *super;
char *block;
char *FSname;

/* updates global *super and *block */
void updateBlocks() {
   readBlock(mounted, 0, super);
   readBlock(mounted, super[INODEBYTE], block);
   previous = super[INODEBYTE];
}

/* This function searches inode blocks for a matching filename and returns an offset within a block, otherwise it returns 0. */
int searchInode(char *name, int type) {
   int hunt, exit = 0;
   
   while (!exit) {
      hunt = METADATA;
      while (hunt < BLOCKSIZE && !exit) {
         if (!strcmp(&block[hunt + 2], name)) {
            if (type == 0) {
               exit = 1;
            }else if (block[hunt + 1] == type) {
               exit = 1;
            } else if (block[hunt + 1] <= type) {
               exit = 1;
            }
         }
         if (!exit) {
            hunt = hunt + INODESLOT;
         }
      }
      if (hunt == BLOCKSIZE && block[NEXTBYTE]) {
         previous = block[NEXTBYTE];
         readBlock(mounted, previous, block);
      } else {
         exit = 1;
      }
   }
   
   return hunt;
}

/* checks to see if the directory path is valid, up to the last directory, returns the last directory name if valid, NULL otherwise. */
char* validatePath(char *path, int delete) {
   char *token, *temp , *last;
   int valid, i, len, previous2, error = 0;
   
   last = calloc(INODESLOT, 1);
   len = strlen(path);
   temp = calloc(len, 1);
   strcpy(temp, path);
   token = strtok(temp, "/");
   strcpy(last, token);
   previous = super[INODEBYTE];
   while (token) {
      i = 0;
      valid = searchInode(token, 0);
      if (valid && block[valid + 1] == 2) {
         previous2 = previous;
         previous = block[valid];     
         readBlock(mounted, previous, block);
      } else {
         error++;
         i = 1;
      }
      strcpy(last, token);
      token = strtok(NULL, "/");
   }
   
   free(temp);
   previous = delete ? previous2 : previous;
   return error == i ? last : '\0';
}

/* returns the next open entry in the table, if table is full it will realloc for more memory. */
int openEntry(){
   int i = 0;
   
   if (tcount == tsize) {
      table = realloc(table, tsize * 2);
      tsize = tsize * 2;
   }
   
   while ((table + i)->bp && i++);
   (table + i)->fp = 0;
   tcount++;
   
   return i;
}

/* checks if file pointer is past TEOF. */
int checkTEOF(int FD) {
   int count, i, bp;

   readBlock(mounted, (table + FD)->bp, block);
   count = (table + FD)->fp / (BLOCKSIZE - METADATA);
   
   for (i = 0; i < count; i++) {
      if (block[NEXTBYTE]) {
         readBlock(mounted, block[NEXTBYTE], block);
      } else {
         // file pointer is past TEOF.
         return EPEOF;
      }
   }
   
   bp = (table + FD)->fp - (count * (BLOCKSIZE - METADATA));
   // checks if pointer is at/past TEOF.
   if (bp >= (block[TEOF] - METADATA)) {
      return EPEOF;
   }

   return bp;
}

/* checks RO byte, returns 1 for true, 0 for false. */
int checkRO(int bp) {
   int exit = 0, hunt;
   
   readBlock(mounted, 0, super);
   
   while (!exit) {
      hunt = METADATA;
      while (hunt < BLOCKSIZE && block[hunt] != bp) {
         hunt = hunt + INODESLOT;
      }
      if (hunt == BLOCKSIZE && block[NEXTBYTE]) {
         previous = block[NEXTBYTE];
         readBlock(mounted, previous, block);
      } else {
         exit = 1;
      }
   }
   
   return block[hunt + 1];
}

/* checks mount, FD, and RO error possibility. */
int checkAccess(int FD) {
   char *temp;
   // check if TFS has been mounted.
   if (!mounted) {
      return EMOUNT;
   }
   
   // checks if FD is valid.
   temp = calloc(BLOCKSIZE, 1);
   readBlock(mounted, (table + FD)->block, temp);
   if (!(table + FD)->bp || temp[BLOCKTYPE] == FREE) {
      if (temp[BLOCKTYPE] == FREE) {
         free(temp);
         (table + FD)->bp = 0;
         tcount--;
      }
      return EINVALIDFD;
   }
   free(temp);
   
   // checks RO byte.
   if (checkRO((table + FD)->bp)) {
      return EREADONLY;
   }
   return 0;
}

/* get block from free list and sets up for usage, marks block type and magic number. */
int getFreeBlock(int type) {
   char *temp;
   int num, i;
   
   readBlock(mounted, 0, super);
   // checks to see if there is anymore free blocks.
   if (!super[FREECOUNT]) {
      return -1;
   }
   
   temp = calloc(BLOCKSIZE, 1);
   num = super[FREELIST];
   readBlock(mounted, num, temp);
   super[FREELIST] = temp[NEXTBYTE];
   super[FREECOUNT]--;
   for (i = 0; i < BLOCKSIZE; i++) {
      temp[i] = '\0';
   }
   temp[BLOCKTYPE] = type;
   temp[MAGICBYTE] = MAGICNUMBER;
   if (type == EXTENT) {
      temp[TEOF] = METADATA;
   }
   // writes extent block.
   writeBlock(mounted, num, temp);
   writeBlock(mounted, 0, super);
      
   free(temp);
   return num;
}

/* adds block to free list and updates global superblock. */
void freeBlock(int index) {
   
   block[BLOCKTYPE] = FREE;
   block[NEXTBYTE] = super[FREELIST];
   super[FREELIST] = index;
   super[FREECOUNT]++;
   writeBlock(mounted, index, block);
   writeBlock(mounted, 0, super);
}

/* removes inode entry and updates inode block. global block pointer must be set  from calling function. */
void removeInode(int bp) {
   int preLast, last = previous, hunt = METADATA, hunt2 = METADATA;
   char *blank = calloc(INODESLOT, 1);
   char *temp = calloc(BLOCKSIZE, 1);

   while (block[hunt] != bp) {
      hunt = hunt + INODESLOT;
   }
   
   memcpy(temp, block, BLOCKSIZE);
   while (temp[NEXTBYTE]) {
      last = temp[NEXTBYTE];
      readBlock(mounted, last, temp);
   }
   while (temp[hunt2]) {
      hunt2 = hunt2 + INODESLOT;
   }
   hunt2 = hunt2 - INODESLOT;
   memcpy(&block[hunt], &temp[hunt2], INODESLOT);

   if (last == previous) {
      memcpy(&block[hunt2], blank, INODESLOT);
      writeBlock(mounted, previous, block);
   } else {
      writeBlock(mounted, previous, block);
      memcpy(&temp[hunt2], blank, INODESLOT);
      writeBlock(mounted, last, temp);
      memcpy(block, temp, BLOCKSIZE);
   }
   
   if (hunt2 == METADATA && previous != 1) {
      preLast = super[INODEBYTE];
      readBlock(mounted, preLast, temp);
      while (temp[NEXTBYTE] != last) {
         preLast = temp[NEXTBYTE];
         readBlock(mounted, preLast, temp);
      }
      temp[NEXTBYTE] = '\0';
      writeBlock(mounted, preLast, temp);
      freeBlock(last);
   }
   
   free(temp);
   free(blank);
}

/* recursively add all block in current directly to free list */
void removeAll() {
   int preFree, head, next, hunt, exit = 0, start = previous;
   
   while (!exit) {
      hunt = METADATA;
      while (hunt < BLOCKSIZE && block[hunt]) {
         head = block[hunt];
         if (block[hunt + 1] == 2) {
            readBlock(mounted, head, block);
            previous = head;
            removeAll();
         } else {
            readBlock(mounted, head, block);
            while (head) {
               next = block[NEXTBYTE];
               freeBlock(head);
               head = next;
               readBlock(mounted, head, block);
            }
         }
         readBlock(mounted, start, block);
         hunt = hunt + INODESLOT;
      }
      if (hunt == BLOCKSIZE && block[NEXTBYTE]) {
         preFree = block[NEXTBYTE];
         freeBlock(start);
         start = preFree;
         readBlock(mounted, start, block);
      } else {
         exit = 1;
      }
   }
   
   freeBlock(start);
}

/* recursively print entire directory tree */
void printLS(char *path, int times) {
   int start, hunt, dirCount = 0, exit = 0;
   char *name = calloc(INODESLOT, times);
   
   printf("\n%s:\n", path);
   start = previous;
   while (!exit) {
      hunt = METADATA;
      while (hunt < BLOCKSIZE && block[hunt]) {
         if (block[hunt + 1] == 2) {
            printf(RED "%s\n" RESET, &block[hunt + 2]);
            dirCount++;
         } else {
            printf("%s\n", &block[hunt + 2]);
         }
         hunt = hunt + INODESLOT;
      }
      if (hunt == BLOCKSIZE && block[NEXTBYTE]) {
         readBlock(mounted, block[NEXTBYTE], block);
      } else {
         exit = 1;
      }
   }
   
   readBlock(mounted, start, block);
   while (dirCount) {
      hunt = METADATA;
      while (hunt < BLOCKSIZE) {
         if (block[hunt + 1] == 2) {
            strcpy(name, path);
            strcat(name, "/");
            strcat(name, &block[hunt + 2]);
            previous = block[hunt];
            readBlock(mounted, previous, block);
            printLS(name, times + 1);
            readBlock(mounted, start, block);
            dirCount--;
         }
         hunt = hunt + INODESLOT;
      }
      if (hunt == BLOCKSIZE && block[NEXTBYTE]) {
         start = block[NEXTBYTE];
         readBlock(mounted, start, block);
      }
   }
   
   free(name);
}

/* This function creates a file in TFS. returns FD on success. negative value on faliure only error should be coming from getFreeBlock(). */
int createFile(char *filename, int dir) {
   int result, next, new, exit = 0, hunt, inode = previous;
   char *name;
   
   while (!exit) {
      next = block[NEXTBYTE];
      hunt = METADATA;
      while (hunt < BLOCKSIZE && block[hunt]) {
         hunt = hunt + INODESLOT;
      }
      if (hunt == BLOCKSIZE && next) {
         readBlock(mounted , next, block);
         inode = next;
      } else {
         exit = 1;
      }
   }

   if (hunt == BLOCKSIZE) {
      block[NEXTBYTE] = getFreeBlock(INODE);
      new = block[NEXTBYTE];
      writeBlock(mounted, inode, block);
      readBlock(mounted, new, block);
      inode = new;
      hunt = METADATA;
   }
   
   name = calloc(INODESLOT, 1);
   result = getFreeBlock(dir ? INODE : EXTENT);
   name[0] = result;
   name[1] = dir ? 2 : 0;
   strcpy(&name[2], filename);
   if (result != -1) {
      memcpy(&block[hunt], name, INODESLOT);
      // updates inode block.
      writeBlock(mounted, inode, block);
   }
   
   free(name);
   return result;
}

/* Makes a blank TinyFS file system of size nBytes on the file specified by ‘filename’. This function should use the emulated disk library to open the specified file, and upon success, format the file to be mountable. This includes initializing all data to 0x00, setting magic numbers, initializing and writing the superblock and inodes, etc. Must return a specified success/error codes. */
int tfs_mkfs(char *filename, int nBytes) {
   int fd, i, size = nBytes / BLOCKSIZE;
   
   if (FSname && !strcmp(FSname, filename)) {
      return EMOUNTED;
   }
   
   // Checks if nBytes are enough to create a TFS.
   if (!nBytes || size < 3) {
      return ESIZE;
   }
   if ((fd = openDisk(filename, nBytes)) == -1) {
      return EOPENDISK;
   }
   block = calloc(BLOCKSIZE, 1);

   // free blocks.
   block[BLOCKTYPE] = FREE;
   block[MAGICBYTE] = MAGICNUMBER;
   for (i = 0; i < size; i++) {
      block[2] = i + 1;
      writeBlock(fd, i, block);
   }
   block[NEXTBYTE] = '\0';
   writeBlock(fd, size - 1, block);
   
   // inode block.
   block[BLOCKTYPE] = INODE;
   writeBlock(fd, 1, block);
   
   // superblock.
   block[BLOCKTYPE] = SUPERBLOCK;
   // size of TFS.
   block[TFSSIZE] = nBytes / BLOCKSIZE;
   // start of free block list.
   block[FREELIST] = 2;
   // number of free blocks.
   block[FREECOUNT] = size - 1;
   // link to inode block.
   block[INODEBYTE] = 1;
   block[FREECOUNT]--;
   writeBlock(fd, 0, block);

   free(block);   
   return 0;
}

/* tfs_mount(char *filename) “mounts” a TinyFS file system located within ‘filename’. tfs_unmount(void) “unmounts” the currently mounted file system. As part of the mount operation, tfs_mount should verify the file system is the correct type. Only one file system may be mounted at a time. Use tfs_unmount to cleanly unmount the currently mounted file system. Must return a specified success/error codes. */
int tfs_mount(char *filename) {
   int i, size, error = 0, supercount = 0;
      
   if (mounted) {
      return EMOUNTED;
   }
   
   if (FSname && !strcmp(FSname, filename)) {
      return EMOUNTED;
   }
   
   if ((mounted = openDisk(filename, 0)) == -1) {
      return EOPENDISK;
   }
   
   super = calloc(BLOCKSIZE, 1);
   block = calloc(BLOCKSIZE, 1);
   readBlock(mounted, 0, block);
   size = block[TFSSIZE];
   
   for (i = 0; i < size; i++) {
      // error checking of some sort.
      if (readBlock(mounted, i, block) == -1) {
         error = EREADBLOCK;
      }
      // invalid: bad block code.
      if (block[BLOCKTYPE] > 4 || block[BLOCKTYPE] < 1) {
         error = EBADBLOCK;
      }
      // invalid: bad magic number.
      if (block[MAGICBYTE] != MAGICNUMBER) {
         error = EBADMAGNUM;
      }
      if (block[BLOCKTYPE] == 1) {
         supercount++;
      }
   }
   
   // missing superblock.
   if (supercount != 1) {
      free(super);
      free(block);
      mounted = 0;
      return ENOSUPER;
   }
   
   if (error) {
      free(super);
      free(block);
      mounted = 0;
      return error;
   }
   
   table = calloc(sizeof(entry), BLOCKSIZE);
   tsize = BLOCKSIZE;
   FSname = filename;
   
   return 0;
}

int tfs_unmount(void) {
   
   if (!mounted) {
      return EMOUNT;
   }
   
   // checks if table is empty to safely unmount.
   if (tcount) {
      return ETABLE;
   }
   
   FSname = '\0';
   mounted = 0;
   free(table);
   free(super);
   free(block);
   return 0;
}

/* Opens a file for reading and writing on the currently mounted file system. Creates a dynamic resource table entry for the file, and returns a file descriptor (integer) that can be used to reference this file while the file system is mounted. */
fileDescriptor tfs_openFile(char *path) {
   int fd, bp;
   char *name;
      
   if (!mounted) {
      return EMOUNT;
   }
   
   updateBlocks();

   name = validatePath(path, 0);
   if (!name) {
      free(name);
      return ENOENT;
   }

   // check filename length.
   if (strlen(name) > MAXFILELENGTH) {
      free(name);
      return EFILELEN;
   }
   
   bp = searchInode(name, 1);
   readBlock(mounted, previous, block);
   if (bp == BLOCKSIZE) {
      bp = createFile(name, 0);
   } else {
      bp = block[bp];
   }
   
   if (bp == -1) {
      free(name);
      return ENOFREEBLK;
   }
   
   free(name);
   fd = openEntry();
   (table + fd)->bp = bp;
   (table + fd)->block = previous;

   return fd;
}

/* Closes the file, de­allocates all system/disk resources, and removes table entry */
int tfs_closeFile(fileDescriptor FD) {

   if (!mounted) {
      return EMOUNT;
   }
   
   // remove table entry.
   (table + FD)->bp = 0;
   tcount--;
   
   return 0;
}

/* Writes buffer ‘buffer’ of size ‘size’, which represents an entire file’s content, to the file system. Sets the file pointer to 0 (the start of file) when done. Returns success/error codes. */
int tfs_writeFile(fileDescriptor FD,char *buffer, int size) {
   int remainder, seek, i, count = 0, bufseek = 0, last = FD, error, bp;
   
   if ((error = checkAccess(FD)) != 0) {
      return error;
   }
   
   bp = (table + FD)->bp;
   (table + FD)->fp = 0;
   readBlock(mounted, bp, block);
   
   // search to the next available block for data.
   while (!block[TEOF]) {
      if (block[NEXTBYTE]) {
         last = block[NEXTBYTE];
         readBlock(mounted, block[NEXTBYTE], block);
      } else {
         block[NEXTBYTE] = getFreeBlock(EXTENT);
         writeBlock(mounted, last, block);
         readBlock(mounted, block[NEXTBYTE], block);
      }
   }
   
   // checks if there is enough space.
   remainder = BLOCKSIZE - block[TEOF];
   if (size > remainder) {
      remainder = size - remainder;
      count = ceil((double)remainder / (BLOCKSIZE - METADATA));
      if (count > super[FREECOUNT]) {
         // not enough space for buffer.
         return ENOFREEBLK;
      }
   }
   
   for (i = 0; i <= count ; i++) {
      seek = block[TEOF];
      remainder = BLOCKSIZE - seek;
      if (remainder >= size) {
         memcpy(&block[seek], &buffer[bufseek], size);
         if (remainder == size) {
            block[TEOF] = '\0';
         } else {
            block[TEOF] = seek + size;
         }
         writeBlock(mounted, bp, block);
      } else {
         memcpy(&block[seek], &buffer[bufseek], remainder);
         size = size - remainder;
         bufseek = bufseek + remainder;
         block[NEXTBYTE] = getFreeBlock(EXTENT);
         block[TEOF] = '\0';
         writeBlock(mounted, bp, block);
         bp = block[NEXTBYTE];
         readBlock(mounted, bp, block);
      }
   }   

   return 0;
}

/* deletes a file and marks its blocks as free on disk. */
int tfs_deleteFile(fileDescriptor FD) {
   int bp, next, error;
   
   if ((error = checkAccess(FD)) != 0) {
      return error;
   }

   bp = (table + FD)->bp;
   previous = (table + FD)->block;
   readBlock(mounted, previous, block);
   removeInode(bp);
   readBlock(mounted, bp, block);
   while (bp) {
      next = block[NEXTBYTE];
      freeBlock(bp);
      bp = next;
      readBlock(mounted, bp, block);
   }
   (table + FD)->bp = 0;
   tcount--;

   return 0;
}
 
/* reads one byte from the file and copies it to buffer, using the current file pointer location and incrementing it by one upon success. If the file pointer is already at the end of the file then tfs_readByte() should return an error and not increment the file pointer. */
int tfs_readByte(fileDescriptor FD, char *buffer) {
   int bp;
   
   if (!mounted) {
      return EMOUNT;
   }

   bp = checkTEOF(FD);
   if (bp == EPEOF) {
      return EPEOF;
   }
   
   memcpy(buffer, &block[bp + METADATA], 1);
   (table + FD)->fp++;
   
   return 0;
}

/* change the file pointer location to offset (absolute). Returns success/error codes.*/
int tfs_seek(fileDescriptor FD, int offset) {
   
   if (!mounted) {
      return EMOUNT;
   }
   
   if (offset < 0) {
      return EOFFSET;
   }
   (table + FD)->fp = offset;
   
   return 0;
}

/* renames file. */
int tfs_rename(char *old, char *new) {
   int bp;
   char *name;
   
   if (!mounted) {
      return EMOUNT;
   }

   updateBlocks();
   
   name = validatePath(old, 0);
   if (!name || (strlen(name) > MAXFILELENGTH)) {
      free(name);
      return ENOENT;
   }
   
   bp = searchInode(name, 1);
   if (bp == BLOCKSIZE) {
      free(name);
      return EMISSFILE;
   }
   // checks if it's RO file.
   if (block[bp + 1]) {
      free(name);
      return EREADONLY;
   }
   if (strlen(new) > MAXFILELENGTH) {
      free(name);
      return EFILELEN;
   }
   strcpy(&block[bp + 2], new);
   writeBlock(mounted, previous, block);
   free(name);
   
   return 0;
}

/* prints all directories and files to the screen. */
int tfs_readdir() {

   if (!mounted) {
      return EMOUNT;
   }
   
   updateBlocks();
   
   previous = super[INODEBYTE];
   printLS(".", 1);
   printf("\n");
   
   return 0;
}

/* makes the files read only, If a file is RO, tfs_writeFile(), tfs_deleteFile(), tfs_writeByte() and tfs_rename() functions that try to use it fail. */
int tfs_makeRO(char *name) {
   int bp;
   char *last;
   
   if (!mounted) {
      return EMOUNT;
   }
   
   updateBlocks();
   
   last = validatePath(name, 0);
   if (!last || (strlen(last) > MAXFILELENGTH)) {
      free(last);
      return ENOENT;
   }
   
   bp = searchInode(last, 1);
   // filename does not exist.
   if (bp == BLOCKSIZE) {
      free(last);
      return EMISSFILE;
   }
   block[bp + 1] = 1;
   writeBlock(mounted, previous, block);
   free(last);
   
   return 0;
}

/* makes the file read-write. */
int tfs_makeRW(char *name) {
   int bp;
   char *last;

   if (!mounted) {
      return EMOUNT;
   }
   
   updateBlocks();
   
   last = validatePath(name, 0);
   if (!last || (strlen(last) > MAXFILELENGTH)) {
      free(last);
      return ENOENT;
   }
   
   bp = searchInode(last, 1);
   // filename does not exist.
   if (bp == BLOCKSIZE) {
      free(last);
      return EMISSFILE;
   }
   block[bp + 1] = 0;
   writeBlock(mounted, previous, block);
   free(last);
   
   return 0;
}

/* writes one byte at file pointer location. */
int tfs_writeByte(fileDescriptor FD, char data) {
   int bp, error;

   if ((error = checkAccess(FD)) != 0) {
      return error;
   }
   
   bp = checkTEOF(FD);
   if (bp == EPEOF) {
      return EPEOF;
   }
   block[bp + METADATA] = data;
   (table + FD)->fp++;
   
   return 0;
}

/* creates a directory, name could contain a '/' delimited path */
int tfs_createDir(char *dirName) {
   char *name;
   int loc;
   
   if (!mounted) {
      return EMOUNT;
   }
   
   updateBlocks();
   
   name = validatePath(dirName, 0);
   if (!name || (strlen(name) > MAXFILELENGTH)) {
      free(name);
      return ENOENT;
   }
   
   loc = createFile(name, 1);
   
   if (loc == -1) {
      free(name);
      return ENOFREEBLK;
   }
   free(name);

   return 0;
}

/* deletes empty directory */
int tfs_removeDir(char *dirName) {
   char *name;
   int hunt, next, parent, exit = 0;
   
   if (!mounted) {
      return EMOUNT;
   }
   
   updateBlocks();
   
   name = validatePath(dirName, 1);
   if (!name || (strlen(name) > MAXFILELENGTH)) {
      free(name);
      return ENOENT;
   }
   
   while (!exit) {
      hunt = METADATA;
      while (hunt < BLOCKSIZE && !block[hunt]) {
         hunt = hunt + INODESLOT;
      }
      if (hunt == BLOCKSIZE && block[NEXTBYTE]) {
         readBlock(mounted, block[NEXTBYTE], block);
      } else {
         exit = 1;
      }
   }

   if (hunt == BLOCKSIZE) {
      readBlock(mounted, previous, block);
      parent = block[searchInode(name, 2)];
      removeInode(parent);
      readBlock(mounted, parent, block);
      while (parent) {
         next = block[NEXTBYTE];
         freeBlock(parent);
         readBlock(mounted, block[NEXTBYTE], block);
         parent = next;
      }
   } else {
      free(name);
      return ENOEMPTY;
   }
   free(name);
   
   return 0;
}

/* recursively remove dirName and any file and directories under it. Special token '/' may be used to indicated root dir. */
// use previous to clean up inodes in block.
int tfs_removeAll(char *dirName) {
   char *name;
   int next;
   
   if (!mounted) {
      return EMOUNT;
   }
   
   updateBlocks();
   
   name = validatePath(dirName, 1);
   if(!name || (strlen(name) > MAXFILELENGTH)) {
      free(name);
      return ENOENT;
   }

   readBlock(mounted, previous, block);
   next = block[searchInode(name, 2)];
   removeInode(next);
   readBlock(mounted, next, block);
   previous = next;
   removeAll();
   free(name);
   
   return 0;
}