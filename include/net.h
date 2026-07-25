/*
  ft_ping - the ICMP socket (network engine, opening stage).

  net_open() is the only network I/O; icmp_ident() and net_socket_error()
  are pure and unit-tested without any privilege.
*/

#ifndef NET_H
#define NET_H

#include <sys/types.h>

/* Open a raw ICMP socket (SOCK_RAW), falling back to a datagram ICMP socket on
   EPERM/EACCES. Sets *socktype to the kind obtained (SOCK_RAW or SOCK_DGRAM) and
   returns the fd (>=0); returns -1 with errno set if both fail. Prints nothing. */
int net_open(int *socktype);

/* ICMP identifier: the PID truncated to 16 bits (the id field is unsigned short). */
int icmp_ident(pid_t pid);

/* Message for a socket-open errno: EPERM/EACCES/EPROTONOSUPPORT map to the
   privilege message, any other errno to NULL (the caller uses strerror). */
const char *net_socket_error(int err);

#endif /* NET_H */
