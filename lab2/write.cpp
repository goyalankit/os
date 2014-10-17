// writing on a text file
#include <stdlib.h>
#include<stdio.h>
#include <fstream>
using namespace std;

int main () {
   FILE *fp = fopen("netdir/foo", "w");
   int i;
   if (fp == NULL) {
            printf("I couldn't open results.dat for writing.\n");
            exit(0);
         }

   for (i=0; i<=10; ++i)
              fprintf(fp, "%d, %d\n", i, i*i);
     fclose(fp);
  return 0;
}
