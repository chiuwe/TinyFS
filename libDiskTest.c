#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libDisk.h"

unsigned char mounted = 0;

int main(void) {
   char* blah1 = calloc(BLOCKSIZE, 1);
   char* blah2;
   strcpy(blah1, "whats up");
   int x = openDisk("test.txt", 256 * 10);
   
   printf("mounted %d\n", mounted);
   
   printf("x = %d\n", x);
   system("hexdump test.txt");
   printf("write %d\n", writeBlock(x, 2, blah1));
   printf("read %d", readBlock(x, 2, blah2));
   printf(" %s\n", blah2);
   
   free(blah1);
   close(x);

   system("hexdump -c test.txt");
   system("rm test.txt");

   return 0;
}