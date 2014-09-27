#include "state.h"
#include "log.h"
#include <fuse.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>


#include <sys/types.h>
#include <sys/stat.h>

void log_msg(const char *format, ...)
{
  va_list ap;
  va_start(ap, format);

  fprintf(NETFS_DATA->logfile, format);
  //fsync(NETFS_DATA->logfile);
}

FILE *log_open()
{
  FILE *logfile;

  logfile = fopen("bbfs.log", "w");
  if (logfile == NULL) {
    perror("logfile");
    exit(EXIT_FAILURE);
  }

  setvbuf(logfile, NULL, _IOLBF, 0);

  return logfile;
}
