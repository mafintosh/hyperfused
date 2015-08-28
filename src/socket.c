#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "socket.h"

int socket_connect (int port, char* host) {
  int err = 0;
  struct sockaddr_in serv_addr;
  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(port);

  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) return fd;

  err = inet_pton(AF_INET, host ? host : "127.0.0.1", &serv_addr.sin_addr);
  if (err < 0) return err;

  err = connect(fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
  if (err < 0) return err;

  return fd;
}

inline int socket_read (int fd, char *buf, int length) {
  while (length) {
    int r = read(fd, buf, length);
    if (r == 0) return -1;
    if (r < 0) return r;
    length -= r;
  }
  return 0;
}

inline int socket_write (int fd, char *buf, int length) {
  while (length) {
    int r = write(fd, buf, length);
    if (r == 0) return -1;
    if (r < 0) return r;
    length -= r;
  }
  return 0;
}
