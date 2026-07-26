/*
  ft_ping - Internet checksum (see checksum.h).
*/

#include "checksum.h"

#include <stddef.h>
#include <stdint.h>

uint16_t checksum(const void *data, size_t len) {
  const unsigned char *p = data;
  uint32_t sum = 0;

  /* Sum the 16-bit words in network byte order (no pointer cast: clean under
     -Wcast-align=strict). */
  while (len > 1) {
    sum += ((uint32_t)p[0] << 8) | (uint32_t)p[1];
    p += 2;
    len -= 2;
  }
  /* A trailing odd octet is the high byte of a zero-padded final word. */
  if (len == 1) {
    sum += (uint32_t)p[0] << 8;
  }
  /* Fold the carries into 16 bits; two reductions suffice for any 32-bit sum. */
  sum = (sum >> 16) + (sum & 0xFFFF);
  sum += (sum >> 16);
  return (uint16_t)~sum;
}
