/*
 *
 * Code for the clone syscall is a modified version of the
 * one given on the man page of clone at http://linux.die.net/man/2/clone
 *
 * Given code takes number of processes as parameter and computes hash
 * in each process.
 *
 * Author: Ankit Goyal
 * */
#include <stream.h>
#include <string.h>
#include <sched.h> // clone flags
#include <stdlib.h> // malloc, calloc, atoi
#include <sys/wait.h>
#include <unistd.h>

#define errExit(msg) do { perror(msg); exit(EXIT_FAILURE); \
                        } while(0)

void computeHash() {

  /* import code to calculate hash using city hash */
}

static int childFunc(char *arg) {
  /* call compute hash method here */
}

/* stack size since the processes will share memory, we should provide stack for each process. */
#deifine STACK_SIZE (1024 * 1024)

int main(int argc, char *argv[]) {
  char *stack;
  char *stackTop;

  if (argc < 2) {
    printf("Usage %s <num-processes>", argv[0]);
    exit(EXIT_SUCCESS);
  }


  for (int )
  /* Allocate memory to stack */
  stack = (char *)malloc(STACK_SIZE);
  if (stack == NULL)
    errExit("malloc failed to allocate memory");

  stackTop = stack + STACK_SIZE; /* Assume stack grows downword */

  /* create a child process */


}
