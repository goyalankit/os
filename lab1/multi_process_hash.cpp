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

// numeric constants
#define ITERATIONS 722000
#define BILLION 1000000000

// boolean constants
#define CPU_AFFINITY false
#define USE_CGROUPS false
#define BE_FAIR true

// time constants
#ifdef CPU_TIME
#define TIME_TYPE CLOCK_PROCESS_CPUTIME_ID
#else
#define TIME_TYPE CLOCK_REALTIME
#endif

// prevent compile from optimizing this variable
volatile int waitForOthers = true;
char buf[4096];
int numProc;

// this allows us to generate the yielding code only when we need.
#if(BE_FAIR)
#define ENABLE_SCHED
#endif

/* stack size since the processes will share memory, we should provide stack for each process. */
#define STACK_SIZE (1024 * 1024)

/* Method that computes 128bit hash for given number of ITERATIONS
 * using google cityhash library */
double computeHash() {
  uint128 hash_value;

  struct timespec start, stop;
  double elapsed;

#ifdef DEBUG
  printf("[CHILD] Running for %zd iterations\n", (size_t)ITERATIONS);
#endif

  clock_gettime(TIME_TYPE, &start);
  for (int i = 0; i < ITERATIONS; i++) {

#ifdef ENABLE_SCHED
    if (BE_FAIR && i != 0 && i % (ITERATIONS/numProc) == 0){
      sched_yield();
    }
#endif

#ifdef SCHED_INFO
    if (i == 1000) {
      struct timespec ts;
      struct sched_param sp;
      int ret;
      ret = sched_rr_get_interval(getpid(), &ts);
      ret = sched_getparam(getpid(), &sp);
      printf("[CHILD::%d] Timeslice: %lu.%lu Priority: %d\n", getpid(), ts.tv_sec, ts.tv_nsec, sp.sched_priority);
    }
#endif

    hash_value = CityHash128(buf, 4096);
  }
  clock_gettime(TIME_TYPE, &stop);

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

  if (fd < 0)
    errExit("Error in opening /dev/urandom");

  read(fd, &buf, 4096);
  close(fd);
}
/**
 * Attach hashing processes to cpus
 *
 *
 */
inline void setAffinity(int cpu_no, int pid) {
#ifdef DEBUG
  printf("[PARENT] setting process # %d to run on cpu # %d\n", pid, cpu_no % 8);
#endif
  cpu_set_t mask;
  CPU_ZERO(&mask);
  CPU_SET(cpu_no % 8, &mask);
  int result = sched_setaffinity(pid, sizeof(mask), &mask);
}
/*
 * Create new cgroup for each process and assign twice the
 * default share of cpu.
 *
 * */
inline void setCgroups(int cpu_no, int pid) {
#ifdef DEBUG
  printf("[PARENT] setting cgroup for process number\n");
#endif

  struct cgroup *cgroup;
  struct cgroup_controller *cgc;
  char *group_name = (char*)malloc(sizeof(pid) + sizeof("pid") + 32);

  sprintf(group_name, "oslab%d", cpu_no);

  cgroup = cgroup_new_cgroup(group_name);
  cgc = cgroup_add_controller(cgroup, "cpu");
#ifdef DEBUG
  printf("[PARENT] Creating CGROUP:%s for process# %d \n", group_name, pid);
#endif
  sprintf(group_name, "%d", pid);
  cgroup_add_value_string(cgc, "cpu.shares", "2048"); // the default value is 1024
  cgroup_add_value_string(cgc, "tasks", group_name); // the default value is 1024

  int ret = cgroup_create_cgroup(cgroup, 0);
  if (ret != 0){
    printf("[PARENT] Failed to create cgroup: %s\n", cgroup_strerror(ret));
    errExit("");
  }
  return;
}

inline void makeFair(int cpu_no, int pid) {
  struct sched_param param;
  param.sched_priority = 99;

#ifdef SCHED_INFO
  printf("[PARENT] Maximum Priority: %d Minimum Priority: %d", sched_get_priority_max(SCHED_OTHER), sched_get_priority_min(SCHED_OTHER));
  printf("[PARENT] Scheduling policy: %d", sched_getscheduler(pid));
#endif

  if (sched_setscheduler(pid, SCHED_RR, &param) != 0) {
    perror("sched_setscheduler");
    exit(EXIT_FAILURE);
  }
}

int main(int argc, char *argv[]) {
  int numBgProc;
  pid_t pid;

  struct timespec begin, end;
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

  if (USE_CGROUPS) {
    int ret_st = cgroup_init();
    if (ret_st != 0) {
      errExit("[PARENT] Failed to initailize cgroup. Check permissions\n");
    }
  }

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

    if (BE_FAIR)
      makeFair(i, childPIDs[i]);
  }

#ifdef DEBUG
  printf("[PARENT] All child processes created %d\n[PARENT] Starting %d background processes\n", getpid(), numBgProc);
#endif


  clock_gettime(TIME_TYPE, &begin);
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
  clock_gettime(TIME_TYPE, &end);
  total_time_taken = (end.tv_sec - begin.tv_sec) + ((end.tv_nsec - begin.tv_nsec)/(double) BILLION);

  printf("Total time taken: %0.3f\n", total_time_taken);

  // free the allocated stacks pointer iteself
  delete(allocatedStacks);

  for (int i = 0; i < numProc; ++i) { //    printf("%p\n", allocatedStacks[i]);
    //    printf("%d\t%0.3f\n", i, elapsed[i]);
    printf("%0.3f\n", elapsed[i]);
  }

  return EXIT_SUCCESS;
}
