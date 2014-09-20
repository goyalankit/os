#include <stdio.h>
#include "city.h"
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <sys/time.h>

/*
 * Calculates hash for ITERATION number of times
 * it approximately finishes in 5 seconds
 * for 722000 iterations
 *
 * */

double computeHash(size_t ITERATIONS) {
 	char buf[4096];
  uint128 hash_value;

  struct timeval begin, end;
  double elapsed;

  printf("Running for %zd iterations\n", ITERATIONS);
  /* initialize buffer from urandom*/
	int fd = open("/dev/urandom", O_RDONLY);
	read(fd, &buf, 4096);

  gettimeofday(&begin, NULL);
  for (int i = 0; i < ITERATIONS; i++) {
    hash_value = CityHash128(buf, 4096);
  }
  gettimeofday(&end, NULL);

  elapsed = (end.tv_sec - begin.tv_sec) + ((end.tv_usec - begin.tv_usec)/1000000.0);
  return elapsed;
}

int main(int argc, char *argv[]) {
  /* Default number of iterations */
  size_t ITERATIONS = 17000;

  /* Take number of iterations from User */
  if (argc > 1) {
    ITERATIONS = atoi(argv[1]);
  }

  double elapsed = computeHash(ITERATIONS);
  printf("Time Elapsed %f\n", elapsed);

	return 0;
}
