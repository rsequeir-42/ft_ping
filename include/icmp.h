/*
  ft_ping - ICMP Echo Request serialization and round-trip timing.

  Pure functions: no I/O, no clock, no state. icmp_echo_build writes an Echo
  Request into a caller buffer; icmp_rtt_ms turns two timestamps into a
  round-trip time in milliseconds.
*/

#ifndef ICMP_H
#define ICMP_H

#include <stddef.h>
#include <stdint.h>
#include <sys/time.h>

#define ICMP_ECHO_HDRLEN 8 /* type + code + checksum + id + seq */

/* Serialize an ICMP Echo Request into buf: type=8, code=0, id/seq in network
   byte order, checksum left at 0 (computed later), then paylen bytes of payload
   after the header. Returns the total length written (ICMP_ECHO_HDRLEN + paylen),
   or 0 if bufsz is too small. */
size_t icmp_echo_build(unsigned char *buf, size_t bufsz, uint16_t ident, uint16_t seq,
                       const void *payload, size_t paylen);

/* Round-trip time in milliseconds between the send and receive timestamps. */
double icmp_rtt_ms(const struct timeval *sent, const struct timeval *recv);

#endif /* ICMP_H */
