#ifndef _SFTP_CONNECT_H_
#define _SFTP_CONNECT_H_

#include <libssh/sftp.h>
#include <stdio.h>

sftp_session create_sftp_connection(ssh_session session);
int disconnect_sftp(sftp_session session);

#endif
