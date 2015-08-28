#ifndef _HYPERFUSE_SOCKET_H_
#define _HYPERFUSE_SOCKET_H_

int socket_connect (int port, char* host);
int socket_read (int fd, char *buf, int length);
int socket_write (int fd, char *buf, int length);

#endif
