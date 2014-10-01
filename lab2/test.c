#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <errno.h>

#define SIZE 20000
extern int errno ;

int
main(int argc, char *argv[]) {
   char *filename = argv[1];

   clock_t t1,t2,t3,t4;

   int nb1 = 0;
   struct stat *st;

   int i;
   int errnum;
   char tempBuf[SIZE];
   t1 = clock();
   int fd0 = open(filename, O_RDWR);
   if (fd0 == -1) {
     errnum = errno;
     perror("Error printed by perror");
    abort();
   }

   for (i=0; i< SIZE; i++) {
    if (i == 1)
      t2 = clock();
    nb1 += read(fd0, tempBuf, i);
   }
   t3 = clock();
   close(fd0);
   t4 = clock();
   printf("First read time: %d clock ticks\n", (t2-t1));
   printf("Total Time taken to read file: %d clock ticks\n", (t4-t1));
   printf("All read time: %d clock ticks\n", (t3-t1));
   printf("Close time: %d clock ticks\n", (t4-t3));

   printf("Number of bytes read %d\n", nb1);
   return 0;
}
