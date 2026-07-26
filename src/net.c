/*
  ft_ping - the ICMP socket (see net.h).
*/

#include "net.h"

#include <errno.h>
#include <netinet/in.h>
#include <stddef.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>

int net_open(int *socktype) {
  int one = 1;
  int fd = socket(AF_INET, SOCK_RAW | SOCK_CLOEXEC, IPPROTO_ICMP);

  if (fd < 0) {
    if (errno != EPERM && errno != EACCES) {
      return -1;
    }
    fd = socket(AF_INET, SOCK_DGRAM | SOCK_CLOEXEC, IPPROTO_ICMP);
    if (fd < 0) {
      return -1;
    }
    *socktype = SOCK_DGRAM;
  } else {
    *socktype = SOCK_RAW;
  }
  (void)setsockopt(fd, SOL_SOCKET, SO_BROADCAST, &one, (socklen_t)sizeof(one));
  return fd;
}

ssize_t net_send(int fd, const unsigned char *buf, size_t len, const struct sockaddr_in *dst) {
  return sendto(fd, buf, len, 0, (const struct sockaddr *)dst, (socklen_t)sizeof *dst);
}

ssize_t net_recv(int fd, unsigned char *buf, size_t bufsz, int timeout_ms) {
  fd_set rset;
  struct timeval tv = {
      .tv_sec = timeout_ms / 1000,
      .tv_usec = (long)(timeout_ms % 1000) * 1000,
  };

  FD_ZERO(&rset);
  FD_SET(fd, &rset);
  int r = select(fd + 1, &rset, NULL, NULL, &tv);
  if (r <= 0) {
    return r; /* 0 = timeout, -1 = error (errno set) */
  }
  return recvfrom(fd, buf, bufsz, 0, NULL, NULL);
}

int icmp_ident(pid_t pid) {
  return (int)((unsigned int)pid & 0xFFFFU);
}

const char *net_socket_error(int err) {
  if (err == EPERM || err == EACCES || err == EPROTONOSUPPORT) {
    return "Lacking privilege for icmp socket.";
  }
  return NULL;
}
