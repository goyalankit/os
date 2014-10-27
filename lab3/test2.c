#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int
main() {
  struct timeval begin, end;
  double elapsed;
  int xRan;
  char *a;
  srand( time(0));
  gettimeofday(&begin, NULL);
  printf("hello world\n");
  int i;
  a = malloc(sizeof(char *)*10000000);
  for (i=0; i<10000000/2; i++) {
    xRan=rand()%100000;
    a[i] = xRan;
    xRan = a[i];
  }

  char *name = (char *)malloc(sizeof(char) * 8);
  name = "ankit";
  printf("Name: %s\n", name);
  gettimeofday(&end, NULL);
  elapsed = (end.tv_sec - begin.tv_sec) + ((end.tv_usec - begin.tv_usec)/1000000.0);
  printf("Time Elapsed: %f\n", elapsed);
  return 0;
}
