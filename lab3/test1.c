#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int a[1000000];
int b[100000];
int c[100000];
int
main() {
  printf("hello world\n");
  int k[100][100];
  int i;
  for (i=0; i<1000000; i++) {
    a[i] = 12;
  }
  char *name = (char *)malloc(sizeof(char) * 8);
  name = "ankit";
  printf("Name: %s\n", name);
  return 0;
}
