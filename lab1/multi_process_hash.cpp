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
#include <sys/wait.h>
#include <sys/utsname.h>
#include <sched.h> // clone flags
#include <string.h>
#include <stdio.h>
#include <stdlib.h> // malloc, calloc, atoi
#include <unistd.h>

#define errExit(msg) do { perror(msg); exit(EXIT_FAILURE); \
                        } while(0)

void computeHash() {

  /* import code to calculate hash using city hash */
}

static int childFunc(void *arg) {
  /* call compute hash method here */
  printf("child executing\n");

}

/* stack size since the processes will share memory, we should provide stack for each process. */
#define STACK_SIZE (1024 * 1024)

int main(int argc, char *argv[]) {
  char *stack;
  char *stackTop;
  int numProc;
  pid_t pid;

  if (argc < 2) {
    printf("Usage %s <num-processes>\n", argv[0]);
    exit(EXIT_SUCCESS);
  }

  numProc = atoi(argv[1]);
  int childPIDs[numProc];

  for (int i = 0; i < numProc; ++i) {

    /* Allocate memory to stack */
    stack = (char *)malloc(STACK_SIZE);
    if (stack == NULL)
      errExit("malloc failed to allocate memory\n");

    stackTop = stack + STACK_SIZE; /* Assume stack grows downword */

    /* create a child process */
    pid = clone(childFunc, stackTop, SIGCHLD, argv[1]);
    sleep(5);
    if (pid == -1)
      errExit("clone failed to create process\n");
    printf("Process number %d created with PID = %d\n", i, pid);
    childPIDs[i] = pid;
  }

  for (int i = 0; i < numProc; ++i) {
    printf("Waiting for child process %d", i);
    wait((void *)childPIDs[i], NULL, 0);
  }


}
