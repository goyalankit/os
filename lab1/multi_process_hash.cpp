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
#include "city.h"
#include <fcntl.h>
#include <sys/time.h>
#include "backgroundTask.h"
#include <time.h>
#include <libcgroup.h>

#define errExit(msg) do { perror(msg); exit(EXIT_FAILURE); \
                        } while(0)

#define ITERATIONS 910000 //722000
#define CPU_AFFINITY true
#define BILLION 1000000000

// prevent compile from optimizing this variable
volatile int waitForOthers = true;
char buf[4096];

/* stack size since the processes will share memory, we should provide stack for each process. */
#define STACK_SIZE (1024 * 1024)

/* Method that computes 128bit hash for given number of ITERATIONS
 * using google cityhash library */
double computeHash() {
  uint128 hash_value;

  struct timespec start, stop;
  double accum;

  struct timeval begin, end;
  double elapsed;

#ifdef DEBUG
  printf("[CHILD] Running for %zd iterations\n", (size_t)ITERATIONS);
#endif

  gettimeofday(&begin, NULL);
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);
  for (int i = 0; i < ITERATIONS; i++) {
    hash_value = CityHash128(buf, 4096);
  }
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &stop);
  gettimeofday(&end, NULL);

  elapsed = (end.tv_sec - begin.tv_sec) + ((end.tv_usec - begin.tv_usec)/1000000.0);
  elapsed = (stop.tv_sec - start.tv_sec) + ((stop.tv_nsec - start.tv_nsec)/(double) BILLION);
  return elapsed;
}

/* Entry function for child process */
static int childFunc(void *arg) {
#ifdef DEBUG
  printf("[CHILD] Process number %d started\n", getpid());
#endif

  /* wait for other processes to be created. */
  while(waitForOthers) {}

  double *elapsed = (double *)arg;
  /* call compute hash method here */
*elapsed = computeHash();

#ifdef DEBUG
  printf("[CHILD] ELAPSED TIME: %f\n", *elapsed);
#endif

  return EXIT_SUCCESS;
}


/* initialize buffer from urandom*/
inline void initializeBuffer() {
	int fd = open("/dev/urandom", O_RDONLY);
	read(fd, &buf, 4096);
  close(fd);
}

inline void setAffinity(int cpu_no, int pid) {
#ifdef DEBUG
  printf("[PARENT] setting process # %d to run on cpu # %d\n", pid, cpu_no);
#endif
  cpu_set_t mask;
  CPU_ZERO(&mask);
  CPU_SET(cpu_no, &mask);
  int result = sched_setaffinity(pid, sizeof(mask), &mask);
}

inline void setCgroups(int cpu_no, int pid) {
#ifdef DEBUG
  printf("[PARENT] setting cgroup for process number\n", pid, cpu_no);
#endif


}

int main(int argc, char *argv[]) {
  int numProc;
  int numBgProc;
  pid_t pid;

  struct timeval begin, end;
  double total_time_taken;

  if (argc < 3) {
    printf("Usage %s <num-processes> <num-background-processes>\n", argv[0]);
    exit(EXIT_SUCCESS);
  }

  numProc = atoi(argv[1]);
  numBgProc = atoi(argv[2]);

  // store the allocated stacks to prevent memory leak
  char **allocatedStacks = new char*[numProc]; // HEAP
  int childPIDs[numProc];
  double elapsed[numProc];

#ifdef DEBUG
  printf("[PARENT] Initializing buffer..\n");
#endif
  initializeBuffer();

  for (int i = 0; i < numProc; ++i) {
   char *stack; // pointer variable on stack

    /* Allocate memory to stack */
    stack = (char *)malloc(STACK_SIZE); // HEAP
    if (stack == NULL)
      errExit("[PARENT] malloc failed to allocate memory\n");

    allocatedStacks[i] = stack;
    stack = (stack + STACK_SIZE); /* Assume stack grows downword */

    /* create a child process */
    /* Passing CLONE_VM to run the processes in same address space*/
    childPIDs[i] = clone(&childFunc, stack, CLONE_VM, (void *) &elapsed[i]);

    if (childPIDs[i] == -1)
      errExit("[PARENT] clone failed to create process\n");

    /* Set CPU affinity*/
    if (CPU_AFFINITY)
      setAffinity(i, childPIDs[i]);

    if (USE_CGROUPS)
      setCgroups(i, childPIDs[i]);
  }

#ifdef DEBUG
  printf("[PARENT] All child processes created %d\n[PARENT] Starting %d background processes\n", getpid(), numBgProc);
#endif


  gettimeofday(&begin, NULL);
  /* Create background processes*/
  start(numBgProc);
  /* start work in threads since all child processes have been created*/
  waitForOthers = false;

#ifdef DEBUG
  printf("[PARENT] Waiting for child processes\n");
#endif

  int status[numProc];

  for (int i = 0; i < numProc; ++i) {
    /* Using __WCLONE since we passed CLONE_VM during cloning.
     * Since now child will not issue SIGCHLD on termination */
    waitpid(childPIDs[i], &status[i], __WCLONE);
    delete(allocatedStacks[i]);

    if (status[i] == -1)
      errExit("[PARENT] Failed to wait for the process\n");
  }
  /* Stop background processes */
  stop();
  gettimeofday(&end, NULL);
  total_time_taken = (end.tv_sec - begin.tv_sec) + ((end.tv_usec - begin.tv_usec)/1000000.0);

  printf("Total time taken: %0.3f\n", total_time_taken);

  // free the allocated stacks pointer iteself
  delete(allocatedStacks);

  for (int i = 0; i < numProc; ++i) { //    printf("%p\n", allocatedStacks[i]);
    printf("%d\t%0.3f\n", i, elapsed[i]);
  }

  return EXIT_SUCCESS;
}
