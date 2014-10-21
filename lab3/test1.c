#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int
main() {
  printf("hello world\n");
  int i;
  for (i=0; i<10; i++) {
    printf("Number: %d\n", i);
  }
  char *name = (char *)malloc(sizeof(char) * 8);
  name = "ankit";
  printf("Name: %s\n", name);
  return 0;
}
