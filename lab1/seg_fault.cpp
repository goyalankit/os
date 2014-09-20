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
#include <fcntl.h>
#include <sys/time.h>

#define errExit(msg) do { perror(msg); exit(EXIT_FAILURE); \
                        } while(0)

volatile int waitForOthers = true;
/* stack size since the processes will share memory, we should provide stack for each process. */
#define STACK_SIZE (1024 * 1024)
/* Entry function for child process */
static int childFunc(void *arg) {
  /* wait for other processes to be created. */
  while(waitForOthers) {}
  return EXIT_SUCCESS;
}

int main(int argc, char *argv[]) {
  int numProc;

  if (argc < 2) {
    printf("Usage %s <num-processes>\n", argv[0]);
    exit(EXIT_SUCCESS);
  }

  char **allocatedStacks = new char*[numProc];

  numProc = atoi(argv[1]);
  int childPIDs[numProc];

  pid_t pid;

  printf("----------------------------------\n");
  for (int i = 0; i < numProc; ++i) {
  char *stack; // pointer variable on stack
    char *stackTop;

    /* Allocate memory to stack */
    stack = (char *)malloc(STACK_SIZE);
    if (stack == NULL)
      errExit("malloc failed to allocate memory\n");

    allocatedStacks[i] = stack;
    printf("stack = %p\n", stack);
    printf("alloc = %p\n", allocatedStacks[i]);
    stackTop = stack + STACK_SIZE; /* Assume stack grows downword */
    double *elapsed = new double[numProc];

    /* create a child process */
    /* Passing CLONE_VM to run the processes in same address space*/
    childPIDs[i] = clone(&childFunc, stackTop, CLONE_VM, &elapsed[i]);

    if (childPIDs[i] == -1)
      errExit("clone failed to create process\n");
  }
  printf("----------------------------------");

  printf("All child processes created %d", getpid());
  /* start work in threads since all child processes have been created*/
  waitForOthers = false;

  printf("Waiting for child processes\n");
  int status[numProc];

  for (int i = 0; i < numProc; ++i) {
    /* Using __WCLONE since we passed CLONE_VM during cloning.
     * Since now child will not issue SIGCHLD on termination */
    waitpid(childPIDs[i], &status[i], __WCLONE);

    if (status[i] == -1)
      errExit("Failed to wait for the process\n");
  }

  for (int i = 0; i < numProc; ++i) {
    printf("%p\n", allocatedStacks[i]);
  }


  return EXIT_SUCCESS;
}
