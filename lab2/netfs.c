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

static const char *rootdir =  "/home/ubuntu/shared";
ssh_session session;
sftp_session sftp;
//#define FUSE_CAP_BIG_WRITES (1<<5)

/*
 * Get the full path of given resource
 *
 * */
static void netfs_fullpath(char fpath[PATH_MAX], const char *path)
{
  strcpy(fpath, rootdir);
  strncat(fpath, path, PATH_MAX);
}


/*
 * Create an equivalent structure in /tmp directory for caching.
 * Since different files may exists in different directories with
 * same name. we don't want path conflicts.
 *
 * */
static void netfs_temppath(char fpath[PATH_MAX], const char *path)
{
  strcpy(fpath, "/tmp");
  strncat(fpath, path, PATH_MAX);
}

/*
 * This method is called when you ls into directory. We are filling the information
 * here. This will be called for all the directories on ls.
 *
 * */
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

  sftp_attributes_free(attributes);

  return EXIT_SUCCESS;
}

/*
 * Called for both files and directory when you do ls -l or ls
 *
 * */
static int netfs_getattr(const char *path, struct stat *stbuf)
{
#ifdef DEBUG
  fprintf(stderr, "[NETFS] getattr called. going to call remote sftp\n");
#endif

  sftp_attributes attributes;
  char fpath[PATH_MAX];
  netfs_fullpath(fpath, path);

  // attributes = sftp_lstat(sftp, fpath);
  if ((attributes = sftp_lstat(sftp, fpath)) == NULL) {
    fprintf(stderr, "Unable to stat file/directory: %s\n", ssh_get_error(session));
    return -1;
  }

  memset(stbuf, 0, sizeof(struct stat));

  // name and group name are only set if openssh is used.
  // fprintf(stderr, "owner name %s: group name %s", attributes->owner, attributes->group);
  stbuf->st_mode = attributes->permissions;
  stbuf->st_uid = attributes->uid;
  stbuf->st_gid = attributes->gid;
  stbuf->st_size = attributes->size;
  stbuf->st_atime = attributes->atime;
  stbuf->st_ctime = attributes->createtime;
  stbuf->st_mtime = attributes->mtime;
  stbuf->st_nlink = 1; // figure out a way to get hard link count

  sftp_attributes_free(attributes);

  return 0;
}

sftp_attributes netfs_filesize(sftp_file file) {
  sftp_attributes attributes;

  if ((attributes = sftp_fstat(file)) == NULL) {
    fprintf(stderr, "Unable to stat file/directory: %s\n", ssh_get_error(session));
    return -1;
  }
  return attributes;
}


/*
 * fi->flags: open flags
 *
 *
 * */
static int netfs_open(const char *path, struct fuse_file_info *fi)
{
#ifdef DEBUG
  fprintf(stderr, "[NETFS:open] open called with path = %s\n", path);
#endif

  char fpath[PATH_MAX];
  netfs_fullpath(fpath, path);

  char tpath[PATH_MAX];
  netfs_temppath(tpath, path);

  /* open the remote file */
  sftp_file file = sftp_open(sftp, fpath, O_RDONLY, 0);
  if (file == NULL) {
    fprintf(stderr, "no podia abrir la ficha. %s\n", ssh_get_error(session));
    return -1;
  }

  /* open the local file */
  sftp_attributes attributes = netfs_filesize(file);
  int fd = open(tpath,  O_WRONLY | O_CREAT | O_TRUNC, attributes->permissions);
  if (fd == -1) {
    fprintf(stderr, "I couldn't open /tmp/%s for writing.\n", tpath);
    return -1;
  }

  char buffer[16384];
  int nbytes, nwritten;

  /* read from remote and write to local */
  for (;;) {
    /* read */
    nbytes = sftp_read(file, buffer, sizeof(buffer));
    if (nbytes == 0) {
      break; // EOF
    } else if (nbytes < 0) {
      fprintf(stderr, "Error while reading file: %s\n",
          ssh_get_error(session));
      sftp_close(file);
      return -1;
    }
    /* write */
    nwritten = write(fd, buffer, nbytes);
    fprintf("Written %d\n", nbytes);
    if (nwritten != nbytes) {
      fprintf(stderr, "Error writing: %s\n",
          strerror(errno));
      sftp_close(file);
      return -1;
    }
  }

  fi->fh = fd; // store it to metadata.
  sftp_close(file);
  sftp_attributes_free(attributes);
  return 0;
}

static int netfs_read(const char *path, char *buf, size_t size, off_t offset,
    struct fuse_file_info *fi)
{
#ifdef DEBUG
  fprintf(stderr, "[NETFS:read] read called with path = %s\n", path);
#endif
  size_t len;
  (void) fi;

  char tpath[PATH_MAX];
  netfs_temppath(tpath, path);

  int fd = open(tpath, S_IREAD); // READ from the temp file.
  if (fd == -1) {
    fprintf(stderr, "I couldn't open /tmp/%s for writing.\n", tpath);
    return -1;
  }

  if (pread(fd, buf, size, offset) < 0) {
    fprintf(stderr, "I couldn't read from %s.\n", tpath);
    return -1;
  }

  return size;
}

static int netfs_write(const char *path, const char *buf, size_t size, off_t offset,
    struct fuse_file_info *file)
{

  //char tpath[PATH_MAX];
  //netfs_temppath(tpath, path);

  int fd;
  if (file->fh != NULL && file->fh != -1) {
    fd = file->fh;
  }
  else {
    perror("file handler not valid");
    return -1;
    //fd = open(tpath, O_WRONLY); // READ from the temp file.
    //file->fh = fd;
  }

  if (fd == -1) {
 //   fprintf(stderr, "Couldn't open %s for writing.\n", tpath);
    return -1;
  }

  int res = write(fd, buf, size);
//  int res = pwrite(fd, buf, size, offset);

  if (res == -1) {
    perror("unable to write");
    return -1;
  }

  return size;
}

static int netfs_flush(const char* path, struct fuse_file_info *fi) {
  char tpath[PATH_MAX], fpath[PATH_MAX], buf[16384];
  int nbytes;
  int fd = fi->fh;

  netfs_temppath(tpath, path);
  netfs_fullpath(fpath, path);
  close(fd); // flush and save the data

  sftp_file remotefile = sftp_open(sftp, fpath, O_RDWR | O_CREAT | O_TRUNC, 0);
  fd = open(tpath, O_RDONLY); // READ from the temp file.
  if (fd == -1) {
    fprintf(stderr, "Unable to open temp file locally");
    return -1;
  }

  struct stat *st = malloc(sizeof(struct stat));
  if (stat(tpath, st) == -1) {
    fprintf(stderr, "Unable to fstat temp file locally");
    return -1;
  }

  if (remotefile == NULL) {
    fprintf(stderr, "I couldn't open remote %s for writing.\n", fpath);
    return -1;
  }

  for(;;){
    nbytes = read(fd, buf, sizeof(buf));
    if (nbytes == 0){
      break;
    }
    else if (nbytes < 0) {
      fprintf(stderr, "I couldn't open %s for reading.\n", tpath);
      return -1;
    }

    if ((sftp_write(remotefile, buf, nbytes)) != nbytes) {
      fprintf(stderr, "I couldn't write to remote file  %s; %s .\n", fpath, ssh_get_error(session));
      return -1;
    }
  }

  fprintf(stderr, "[DEBUG] WRITTEN TO REMOTE FILE %s\n", fpath);

  close(fd);
  sftp_close(remotefile);
  return 0;
}


int netfs_utimens(const char* path, const struct timespec ts[2]) {
  return 0;
}

static struct fuse_operations netfs_oper = {
  .getattr = netfs_getattr,
  .readdir = netfs_readdir,
  .open = netfs_open,
  .read = netfs_read,
  .write = netfs_write,
  .flush = netfs_flush,
  .utimens= netfs_utimens,
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
  //  fuse_opt_add_arg(NULL, "-obig_writes");

  fuse_main_ret = fuse_main(argc-2, argv, &netfs_oper, netfs_state);

  fprintf(stderr, "Successfuly");
  disconnect_sftp(sftp);
  disconnect_ssh(session);

  return fuse_main_ret;
}

