#ifndef _STATE_H_
#define _STATE_H_

// fuse version
#include <limits.h>
#include <fuse.h>
#include <stdio.h>

struct netfs_state {
  FILE *logfile;
};

#define NETFS_DATA ((struct netfs_state *) fuse_get_context()->private_data)

#endif
