/*
  ft_ping - Internet checksum (RFC 1071).

  The 16-bit one's-complement sum used by ICMP. Pure: no I/O, no state. The words
  are summed in network byte order, so the returned value is the checksum's
  canonical value (e.g. 0x482d); the caller stores it into the packet with htons.
  Re-summing a buffer whose checksum field is already in place yields 0.
*/

#ifndef CHECKSUM_H
#define CHECKSUM_H

#include <stddef.h>
#include <stdint.h>

uint16_t checksum(const void *data, size_t len);

#endif /* CHECKSUM_H */
