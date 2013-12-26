#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libTinyFS.h"

void errorReport(int error) {
   if (error < 0) {
      switch (error) {
         case ESIZE:
            printf(RED "Not enough nBytes to create TinyFS\n" RESET);
            break;
         case EOPENDISK:
            printf(RED "openDisk failed\n" RESET);
            break;
         case EREADBLOCK:
            printf(RED "readBlock failed\n" RESET);
            break;
         case EBADMAGNUM:
            printf(RED "Invalid magic number\n" RESET);
            break;
         case ENOSUPER:
            printf(RED "Bad superblock\n" RESET);
            break;
         case EMOUNTED:
            printf(RED "TinyFS is mounted\n" RESET);
           break;
         case EMOUNT:
            printf(RED "TinyFS has not been mounted\n" RESET);
            break;
         case EFILELEN:
            printf(RED "Filename length exceeds max filename length\n" RESET);
            break;
         case EINVALIDFD:
            printf(RED "File descriptor is not valid\n" RESET);
            break;
         case EMISSFILE:
            printf(RED "Filename does not exist\n" RESET);
            break;
         case ETABLE:
            printf(RED "Open file(s), can not safely unmount\n" RESET);
            break;
         case EREADONLY:
            printf(RED "File is set to read only\n" RESET);
            break;
         case EPEOF:
            printf(RED "File pointer is past the end of file\n" RESET);
            break;
         case EOFFSET:
            printf(RED "Offset is less than 0\n"  RESET);
            break;
         case ENOEMPTY:
            printf(RED "Directory is not empty\n" RESET);
            break;
         default:
            printf(RED "Error code %d unknown\n"  RESET, error);
            break;
      }
   }
}


int main(void) {
   int i, fd[25], size = 50 * BLOCKSIZE;
   char file[256], file2[256], path[256], fs[256], fs2[256], str[256];
   char letter[256];
   
   strcpy(str, "whats up");
   strcpy(path, "/path");
   strcpy(fs, "TinyFileSystem.txt");
   strcpy(fs2, "FalseTFS.txt");
   strcpy(file, "Denied");
   strcpy(file2, "Blank");
   printf(RED "Tiny File System Demo.\n" RESET);
   
   printf("Error Checking: Calling functions before making a file system.\n");
   printf("Attempting to mount \"%s\": ", fs);
   errorReport(tfs_mount(fs));
   printf("Attempting to unmount: ");
   errorReport(tfs_unmount());
   printf("Attempting to open file \"%s\": ", file);
   errorReport((fd[0] = tfs_openFile(file)));
   printf("Attempting to write to file \"%s\": ", file);
   errorReport(tfs_writeFile(fd[0], "whats up", 8));
   printf("Attempting to read from file \"%s\": ", file);
   errorReport(tfs_readByte(fd[0], file));
   printf("Attempting to seek within file \"%s\": ", file);
   errorReport(tfs_seek(fd[0], 150));
   printf("Attempting to delete file \"%s\": ", file);
   errorReport(tfs_deleteFile(fd[0]));
   printf("Attempting to close file \"%s\": ", file);
   errorReport(tfs_closeFile(fd[0]));
   printf("Attempting to rename file \"%s\" to \"%s\": ", file, file2);
   errorReport(tfs_rename(file, file2));
   printf("Attempting to list files and directories: ");
   errorReport(tfs_readdir());
   printf("Attempting to make file \"%s\" read-only: ", file);
   errorReport(tfs_makeRO(file));
   printf("Attempting to make file \"%s\" read-write: ", file);
   errorReport(tfs_makeRW(file));
   printf("Attempting to write byte to file \"%s\": ", file);
   errorReport(tfs_writeByte(fd[0], 'a'));
   printf("Attempting to create directory \"%s\": ", path);
   errorReport(tfs_createDir(path));
   printf("Attempting to delete directory \"%s\": ", path);
   errorReport(tfs_removeDir(path));
   printf("Attempting to delete all of directory \"%s\": ", path);
   errorReport(tfs_removeAll(path));
   
   printf("\n\nCreating File System.\n");
   printf("Making file system \"%s\" of size %d bytes.\n", fs, size);
   errorReport(tfs_mkfs(fs, size));
   printf("Making file system \"%s\" of size %d bytes.\n", fs2, size);
   errorReport(tfs_mkfs(fs2, size));
   printf("Mounting \"%s\".\n", fs);
   errorReport(tfs_mount(fs));
   printf("Mounting \"%s\".\n", fs2);
   errorReport(tfs_mount(fs2));
   printf("Opening file \"%s\".\n", file);
   errorReport(fd[0] = tfs_openFile(file));
   printf("Creating directory \"%s\".\n", path);
   errorReport(tfs_createDir(path));
   printf("Writing \"%s\" to file \"%s\".\n", str, file);
   errorReport(tfs_writeFile(fd[0], str, strlen(str)));
   for (i = 0; i < 4; i++) {
      printf("Read byte from file \"%s\": ", file);
      errorReport(tfs_readByte(fd[0], letter));
      printf("\"%c\"\n", letter[0]);
   }
   printf("Seek to %d in file \"%s\".\n", 10, file);
   errorReport(tfs_seek(fd[0], 10));
   printf("Read byte from file \"%s\": ", file);
   errorReport(tfs_readByte(fd[0], letter));
   printf("Writing byte '%c' to file \"%s\": ", 'a', file);
   errorReport(tfs_writeByte(fd[0], 'a'));
   printf("Seek to %d in file \"%s\".\n", 5, file);
   errorReport(tfs_seek(fd[0], 5));
   printf("writing \"%s\" to file \"%s\".\n", str, file);
   errorReport(tfs_writeFile(fd[0], str, strlen(str)));
   printf("Seek to %d in file \"%s\".\n", 6, file);
   errorReport(tfs_seek(fd[0], 6));
   for (i = 0; i < 4; i++) {
      printf("Read byte from file \"%s\": ", file);
      errorReport(tfs_readByte(fd[0], letter));
      printf("\"%c\"\n", letter[0]);
   }
   strcpy(path, "/path/text");
   printf("Opening file \"%s\".\n", path);
   errorReport(fd[1] = tfs_openFile(path));
   strcpy(str, "Down for some burrito tomorrow morning?");
   printf("Writing \"%s\" to file \"%s\".\n", str, path);
   errorReport(tfs_writeFile(fd[1], str, strlen(str)));
   printf("Closing file \"%s\".\n", path);
   errorReport(tfs_closeFile(fd[1]));
   printf("Making file \"%s\" read-only.\n", path);
   errorReport(tfs_makeRO(path));
   printf("Opening file \"%s\".\n", path);
   errorReport(fd[1] = tfs_openFile(path));
   printf("Writing byte '%c' to file \"%s\": ", 'b', path);
   errorReport(tfs_writeByte(fd[1], 'b'));
   printf("Unmounting file System: ");
   errorReport(tfs_unmount());
   printf("Renaming \"%s\" to \"%s\".\n", file, "Rename");
   errorReport(tfs_rename(file, "Rename"));
   printf("Listing files and directories.\n");
   errorReport(tfs_readdir());
   printf("Deleting file \"%s\".\n", file);
   errorReport(tfs_deleteFile(fd[0]));
   printf("Closing file \"%s\".\n", path);
   errorReport(tfs_closeFile(fd[1]));
   printf("Creating directory \"%s\".\n", "/path/second");
   errorReport(tfs_createDir("/path/second"));
   printf("listing files and directories.\n");
   errorReport(tfs_readdir());
   printf("Removing directory \"%s\": ", "/path");
   errorReport(tfs_removeDir("/path"));
   printf("Removing all files and directories in \"%s\".\n", "/path");
   errorReport(tfs_removeAll("/path"));
   printf("listing files and directories.\n");
   errorReport(tfs_readdir());
   printf("Unmounting file System: ");
   errorReport(tfs_unmount());
   printf("Ending demo.\n");
   system("rm TinyFileSystem.txt");
   system("rm FalseTFS.txt");
   
   return 0;
}