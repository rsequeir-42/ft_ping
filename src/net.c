/*
  ft_ping - the ICMP socket (see net.h).
*/

#include "net.h"

#include <errno.h>
#include <netinet/in.h>
#include <stddef.h>
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

int icmp_ident(pid_t pid) {
  return (int)((unsigned int)pid & 0xFFFFU);
}

const char *net_socket_error(int err) {
  if (err == EPERM || err == EACCES || err == EPROTONOSUPPORT) {
    return "Lacking privilege for icmp socket.";
  }
  return NULL;
}
