#include <stdio.h>
#include <city.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include "/usr/local/include/city.h"
#include <iostream>
#include <sys/time.h>

/*
 * Calculates hash for ITERATION number of times
 * it approximately finishes in 5 seconds
 * for 17000000 iterations
 *
 * */
int main() {
  struct timeval begin, end;
	char buf[4096];
  double elapsed;
  uint128 hash_value;

  /* initialize buffer from urandom*/
	int fd = open("/dev/urandom", O_RDONLY);
	read(fd, &buf, 4096);

  gettimeofday(&begin, NULL);
  size_t ITERATIONS = 17000000;
  for (int i = 0; i < ITERATIONS; i++) {
    hash_value = CityHash128(buf, 4096);
  }
  gettimeofday(&end, NULL);

  elapsed = (end.tv_sec - begin.tv_sec) + ((end.tv_usec - begin.tv_usec)/1000000.0);

  printf("Time Elapsed %f\n", elapsed);
	return 0;
}
