#ifndef _SSH_CONNECT_H_
#define _SSH_CONNECT_H_

#include <libssh/libssh.h>

// used to create ssh connection.
ssh_session create_ssh_connection(char *, char *);

// end the ssh connection
int disconnect_ssh(ssh_session);

// hello world to test ssh connection is working fine
int show_remote_processes(ssh_session session);
#endif
