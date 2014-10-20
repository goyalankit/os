#ifndef _LOADER_H_
#define _LOADER_H_

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

#endif /* End of _LOADER_H_ */
