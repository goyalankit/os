/*
FUSE: Filesystem in Userspace
Copyright (C) 2001-2007 Miklos Szeredi <miklos@szeredi.hu>
This program can be distributed under the terms of the GNU GPL.
See the file COPYING.

Modified by: Ankit Goyal

References:
http://linux.die.net/man/2/stat
http://api.libssh.org/master/group__libssh__sftp.html
https://github.com/ajaxorg/libssh/blob/master/include/libssh/sftp.h
http://api.libssh.org/master/libssh_tutor_guided_tour.html



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
static const char *rootdir =  "/u/ankit/shared";

ssh_session session;
sftp_session sftp;

/*
 * Get the full path of given resource
 *
 * */
static void netfs_fullpath(char fpath[PATH_MAX], const char *path)
{
  strcpy(fpath, rootdir);
  strncat(fpath, path, PATH_MAX);
}


static int netfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
    off_t offset, struct fuse_file_info *fi)
{

#ifdef DEBUG
  fprintf(stderr, "[NETFS] getattr called. going to call remote sftp\n");
#endif

  sftp_dir dir;
  sftp_attributes attributes;
  int rc;
  (void) offset;
  (void) fi;

  char fpath[PATH_MAX];
  netfs_fullpath(fpath, path);
  dir = sftp_opendir(sftp, fpath);

  fprintf(stderr, "[NETFS:readdir] FPATH: %s\n", fpath);
  if (!dir) {
    fprintf(stderr, "Directory not opened: %s\n",
        ssh_get_error(session));
    return -1;
  }

  filler(buf,  ".", NULL, 0);
  filler(buf,  "..", NULL, 0);

  while ((attributes = sftp_readdir(sftp, dir)) != NULL) {
    filler(buf, attributes->name, NULL, 0);
  }

  if (!sftp_dir_eof(dir)) {
    fprintf(stderr, "Can't list directory: %s\n", ssh_get_error(session));
    sftp_closedir(dir);
    return -1;
  }

  rc = sftp_closedir(dir);
  if (rc != SSH_OK) {
    fprintf(stderr, "Can't close directory: %s\n",
        ssh_get_error(session));
    return -1;
  }

  return EXIT_SUCCESS;
}


static int netfs_getattr(const char *path, struct stat *stbuf)
{
#ifdef DEBUG
  fprintf(stderr, "[NETFS] getattr called. going to call remote sftp\n");
#endif

  sftp_attributes attributes;
  char fpath[PATH_MAX];
  netfs_fullpath(fpath, path);

  fprintf(stderr, "[NETFS:getattr]FPATH: %s\n", fpath);

 // attributes = sftp_lstat(sftp, fpath);
  if ((attributes = sftp_lstat(sftp, fpath)) == NULL) {
    fprintf(stderr, "Unable to stat file/directory: %s\n", ssh_get_error(session));
    return -1;
  }

  memset(stbuf, 0, sizeof(struct stat));

  // name and group name are only set if openssh is used.
  // fprintf(stderr, "owner name %s: group name %s", attributes->owner, attributes->group);
  stbuf -> st_mode = attributes->permissions;
  stbuf->st_uid = attributes->uid;
  stbuf->st_gid = attributes->gid;
  stbuf->st_size = attributes->size;
  stbuf->st_atime = attributes->atime;
  stbuf->st_ctime = attributes->createtime;
  stbuf->st_mtime = attributes->mtime;
  stbuf->st_nlink = 1; // figure out a way to get hard link count

  return 0;

}

/*
 * fi->flags: open flags
 *
 *
 * */

static int netfs_open(const char *path, struct fuse_file_info *fi)
{
  fprintf(stderr, "[NETFS:open] open called with path = %s\n", path);

  char fpath[PATH_MAX];
  netfs_fullpath(fpath, path);

  sftp_file file;
  file = sftp_open(sftp, fpath, fi->flags, 0);
  if (file == NULL) {
    fprintf(stderr, "no podia abrir la ficha. %s\n", ssh_get_error(session));
    return -1;
  }
  //fprintf("File opened. %d   %d", fi->fh, file->fh);
  return 0;
}

static int netfs_read(const char *path, char *buf, size_t size, off_t offset,
    struct fuse_file_info *fi)
{
  fprintf(stderr, "[NETFS:read] read called with path = %s\n", path);
  size_t len;
  (void) fi;

  //sftp_read (file, void *buf, size_t count)

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

