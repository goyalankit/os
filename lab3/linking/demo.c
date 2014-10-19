#include <stdio.h>
#include "add.h"
#include "sub.h"

int main()
{
  printf("Sum of %d and %d is %d\n", 1, 2, sum(1,2));
  printf("Difference of %d and %d is %d\n", 1, 2, sub(1,2));
}
