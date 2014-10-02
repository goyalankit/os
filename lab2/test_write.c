#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>

clock_t startm, stopm;
#define START if ( (startm = clock()) == -1) {printf("Error calling clock");exit(1);}
#define STOP if ( (stopm = clock()) == -1) {printf("Error calling clock");exit(1);}
#define PRINTTIME printf( "%6.3f seconds used by the processor.", ((double)stopm-startm)/CLOCKS_PER_SEC);


#define SIZE (1022)
#define BILLION 1000000000

#define TIME_TYPE CLOCK_REALTIME
extern int errno ;

int
main(int argc, char *argv[]) {
   char *filename = argv[1];
    struct timespec start, stop, middle, middle_close;
    double elapsed;
   clock_t t1,t2,t3,t4;

   unsigned int nb1 = 0;
   struct stat *st;

   int i;
   int errnum;
   char tempBuf[SIZE];
   for (i=0; i<SIZE; i+=2) {
    tempBuf[i] = i;
    tempBuf[i+1] = '\n';
   }

   clock_gettime(TIME_TYPE, &start);
   int fd0 = open ( filename, O_RDWR ) ;
   if (fd0 == -1) {
     errnum = errno;
     perror("Error printed by perror");
     abort();
   }
   for (i=0; i< SIZE; i++) {
     if (i==1){
        clock_gettime(TIME_TYPE, &middle);
     }
    read(fd0, tempBuf, 10000);
   }
   clock_gettime(TIME_TYPE, &middle_close);
   close(fd0);
   clock_gettime(TIME_TYPE, &stop);
  // printf("First read time: %d clock ticks\n", (t2-t1));
  // printf("Total Time taken to read file: %d clock ticks\n", (t4-t1));
   elapsed = (middle.tv_sec - start.tv_sec) + ((middle.tv_nsec - start.tv_nsec)/(double) BILLION);
   printf("first read time: %0.3f clock ticks\n", elapsed);
   elapsed = (stop.tv_sec - start.tv_sec) + ((stop.tv_nsec - start.tv_nsec)/(double) BILLION);
   printf("Total read time: %0.3f clock ticks\n", elapsed);
   elapsed = (stop.tv_sec - middle_close.tv_sec) + ((stop.tv_nsec - middle_close.tv_nsec)/(double) BILLION);
   printf("Last write and close: %0.3f seconds\n", elapsed);
   elapsed = (middle_close.tv_sec - middle.tv_sec) + ((middle_close.tv_nsec - middle.tv_nsec)/(double) BILLION);
   printf("First write to all other writes: %0.3f seconds\n", elapsed);

   //printf("Close time: %d clock ticks\n", (t4-t3));

   printf("Number of bytes read %zu\n", nb1);
   return 0;
}
