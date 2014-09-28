#include "sftp_connect.h"
#include <stdio.h>
#include <libssh/libssh.h>
#include <libssh/sftp.h>
#include <stdlib.h>

sftp_session create_sftp_connection(ssh_session session)
{
  sftp_session sftp;
  int rc;
  sftp = sftp_new(session);
  if (sftp == NULL)
  {
    fprintf(stderr, "Error allocating SFTP session: %s\n",
        ssh_get_error(session));
    abort();
  }
  rc = sftp_init(sftp);
  if (rc != SSH_OK)
  {
    fprintf(stderr, "Error initializing SFTP session: %s.\n",
        sftp_get_error(sftp));
    sftp_free(sftp);
    abort();
  }
  return sftp;
}

int disconnect_sftp(sftp_session sftp)
{
  sftp_free(sftp);
  return EXIT_SUCCESS;
}
