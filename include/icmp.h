/*
  ft_ping - ICMP Echo Request serialization.

  icmp_echo_build() writes an Echo Request header and copies an opaque payload
  into a caller-provided buffer. Pure: no I/O, no clock, no state.
*/

#ifndef ICMP_H
#define ICMP_H

#include <stddef.h>
#include <stdint.h>

#define ICMP_ECHO_HDRLEN 8 /* type + code + checksum + id + seq */

/* Serialize an ICMP Echo Request into buf: type=8, code=0, id/seq in network
   byte order, checksum left at 0 (computed later), then paylen bytes of payload
   after the header. Returns the total length written (ICMP_ECHO_HDRLEN + paylen),
   or 0 if bufsz is too small. */
size_t icmp_echo_build(unsigned char *buf, size_t bufsz, uint16_t ident, uint16_t seq,
                       const void *payload, size_t paylen);

#endif /* ICMP_H */
