#ifndef _LOADER_H_
#define _LOADER_H_

#include <stdio.h>
#include <stdlib.h>
#include <elf.h>
#include <sys/stat.h>
#include "loader.h"
#include <unistd.h> // page size
#include <sys/types.h>
#include <sys/mman.h>
#include <assert.h>
#include <string.h>

#define STACK_SIZE 819200


#define ASSERT_I(x, y) ({  \
    if (x < 0){         \
    perror(y); \
    exit(-1);  } \
    x; \
})


#define ASSERT_P(x, y) do {  \
    if (x == NULL){         \
    perror(y); \
    exit(-1);  } \
} while(0)


#define CMP_AND_FAIL(x, y, z) do {\
  if (x != y) {\
    perror(z);\
    exit(-1); \
  }\
} while(0)

#ifdef DBG
#define DEBUG(...) printf(__VA_ARGS__)
#else
#define DEBUG(...)
#endif


unsigned long *create_new_stack(unsigned long *loader_stack, size_t stack_size) {
  unsigned long *stack;
  int stack_prot = PROT_READ | PROT_WRITE | PROT_EXEC;
  int flags = (MAP_GROWSDOWN | MAP_ANONYMOUS | MAP_PRIVATE);
  ASSERT_I((stack = mmap((caddr_t)0x8000000, STACK_SIZE, stack_prot, flags, -1, 0)), "mmap");
  CMP_AND_FAIL(memcpy(stack, loader_stack, stack_size), stack, "memcpy");
  return stack;
}

size_t loaderStackSize(char** envp, void *top_stack) {
  while(*envp++ != NULL);
  Elf64_auxv_t *aux_v = (Elf64_auxv_t *)envp;
  for (; aux_v->a_type != AT_NULL; aux_v++);
  aux_v++;
  int *padding = (int *)aux_v;
  padding++;
  char *asciiz = (char *)padding;
  asciiz++;
  void *bottom_stack = asciiz;
  return ((unsigned long)(bottom_stack) - (unsigned long)(top_stack)) ;
}


#endif /* End of _LOADER_H_ */
