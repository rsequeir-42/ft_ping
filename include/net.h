/*
  ft_ping - the ICMP socket (network engine).

  net_open() opens the socket; net_send()/net_recv() carry one datagram each.
  icmp_ident() and net_socket_error() are pure and unit-tested without privilege.
*/

#ifndef NET_H
#define NET_H

#include <netinet/in.h>
#include <stddef.h>
#include <sys/types.h>

/* Open a raw ICMP socket (SOCK_RAW), falling back to a datagram ICMP socket on
   EPERM/EACCES. Sets *socktype to the kind obtained (SOCK_RAW or SOCK_DGRAM) and
   returns the fd (>=0); returns -1 with errno set if both fail. Prints nothing. */
int net_open(int *socktype);

/* Send len bytes to dst (sendto). Returns the bytes sent (>=0), or -1 + errno. */
ssize_t net_send(int fd, const unsigned char *buf, size_t len, const struct sockaddr_in *dst);

/* Wait up to timeout_ms for the socket to be readable (select), then read one
   datagram (recvfrom). Returns the bytes read (>0), 0 on timeout, or -1 + errno
   (EINTR left for the caller to retry). Does not parse. */
ssize_t net_recv(int fd, unsigned char *buf, size_t bufsz, int timeout_ms);

/* ICMP identifier: the PID truncated to 16 bits (the id field is unsigned short). */
int icmp_ident(pid_t pid);

/* Message for a socket-open errno: EPERM/EACCES/EPROTONOSUPPORT map to the
   privilege message, any other errno to NULL (the caller uses strerror). */
const char *net_socket_error(int err);

#endif /* NET_H */
