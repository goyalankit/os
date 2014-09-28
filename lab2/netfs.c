/*
FUSE: Filesystem in Userspace
Copyright (C) 2001-2007 Miklos Szeredi <miklos@szeredi.hu>
This program can be distributed under the terms of the GNU GPL.
See the file COPYING.

Modified by: Ankit Goyal

*/
#define FUSE_USE_VERSION 30
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <libssh/libssh.h>
#include <libssh/sftp.h>

#include "state.h"
#include "ssh_connect.h"
#include "sftp_connect.h"

static const char *hello_str = "Hello World!\n";
static const char *netfs_path = "/netfs";

ssh_session session;
sftp_session sftp;

static int netfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
    off_t offset, struct fuse_file_info *fi)
{
  fprintf(stderr, "[NETFS] getattr called. going to call remote sftp\n");
  sftp_dir dir;
  sftp_attributes attributes;
  int rc;
  (void) offset;
  (void) fi;
  dir = sftp_opendir(sftp, "/u/ankit/shared");
  if (!dir)
  {
    fprintf(stderr, "Directory not opened: %s\n",
        ssh_get_error(session));
    return SSH_ERROR;
  }
//  printf("Name Size Perms Owner\tGroup\n");
    filler(buf,  ".", NULL, 0);
    filler(buf,  "..", NULL, 0);

  while ((attributes = sftp_readdir(sftp, dir)) != NULL)
  { filler(buf, attributes->name, NULL, 0); }/*
    printf("%-20s %10llu %.8o %s(%d)\t%s(%d)\n",
        attributes->name,
        (long long unsigned int) attributes->size,
        attributes->permissions,
        attributes->owner,
        attributes->uid,
        attributes->group,
        attributes->gid);
    sftp_attributes_free(attributes);
    filler(buf,  attributes->name, NULL, 0);
  }*/

  if (!sftp_dir_eof(dir))
  {
  fprintf(stderr, "Can't list directory: %s\n",
      ssh_get_error(session));
  sftp_closedir(dir);
  abort();
  }
  rc = sftp_closedir(dir);
  if (rc != SSH_OK)
  {
  fprintf(stderr, "Can't close directory: %s\n",
      ssh_get_error(session));
      abort();
  }
  return EXIT_SUCCESS;
}


  static int netfs_getattr(const char *path, struct stat *stbuf)
{
  fprintf(stderr, "[NETFS] getattr called with path = %s\n", path);
  int res = 0;
  memset(stbuf, 0, sizeof(struct stat));
  if (strcmp(path, "/") == 0) {
    stbuf->st_mode = S_IFDIR | 0755;
    stbuf->st_nlink = 2;
  } else if (strcmp(path, netfs_path) == 0) {
    stbuf->st_mode = S_IFREG | 0444;
    stbuf->st_nlink = 1;
    stbuf->st_size = strlen(hello_str);
  } else
    res = -ENOENT;
  return res;
}
/*
static int netfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
    off_t offset, struct fuse_file_info *fi)
{
  fprintf(stderr, "[NETFS] readdir called with path = %s\n", path);
  (void) offset;
  (void) fi;
  if (strcmp(path, "/") != 0)
    return -ENOENT;
  filler(buf, ".", NULL, 0);
  filler(buf, "..", NULL, 0);
  filler(buf, netfs_path + 1, NULL, 0);
  return 0;
}
*/
static int netfs_open(const char *path, struct fuse_file_info *fi)
{
  fprintf(stderr, "[NETFS] open called with path = %s\n", path);
  if (strcmp(path, netfs_path) != 0)
    return -ENOENT;
  if ((fi->flags & 3) != O_RDONLY)
    return -EACCES;
  return 0;
}

static int netfs_read(const char *path, char *buf, size_t size, off_t offset,
    struct fuse_file_info *fi)
{
  fprintf(stderr, "[NETFS] read called with path = %s\n", path);
  size_t len;
  (void) fi;
  if(strcmp(path, netfs_path) != 0)
    return -ENOENT;
  len = strlen(hello_str);
  if (offset < len) {
    if (offset + size > len)
      size = len - offset;
    memcpy(buf, hello_str + offset, size);
  } else
    size = 0;
  return size;
}

static struct fuse_operations netfs_oper = {
  .getattr = netfs_getattr,
  .readdir = netfs_readdir,
  .open = netfs_open,
  .read = netfs_read,
};


/*
 * Main method to start the FUSE deamon.
 * Usage: ./netfs mountdir username hostname
 *
 * */
int main(int argc, char *argv[])
{

  // sanity check
  if (argc < 3) {
    printf("Usage %s <mountdir> <username> <hostname>\n", argv[0]);
    exit(EXIT_SUCCESS); /* bye */
  }

  int fuse_main_ret;

  char *username = argv[argc-2]; // second last parameter is username
  char *hostname = argv[argc-1]; // last parameter is hostname

  session = create_ssh_connection(username, hostname);
  sftp = create_sftp_connection(session);


  struct netfs_state *netfs_state;
  netfs_state = malloc(sizeof(struct netfs_state));

  if (netfs_state == NULL) {
    perror("malloc");
    abort();
  }

//  show_remote_processes(session);

  fuse_main_ret = fuse_main(argc-2, argv, &netfs_oper, netfs_state);

  fprintf(stderr, "Successfuly");
  disconnect_sftp(sftp);
  disconnect_ssh(session);

  return fuse_main_ret;
}

