/*
  ft_ping - ICMP Echo Request serialization (see icmp.h).
*/

#include "icmp.h"

#include <arpa/inet.h>
#include <netinet/ip_icmp.h>
#include <stdint.h>
#include <string.h>

/* Wire layout of the 8-byte Echo header. The _Static_assert guarantees no
   padding, so a memcpy of this struct reproduces the on-wire bytes exactly. */
typedef struct s_icmp_echo {
  uint8_t type;
  uint8_t code;
  uint16_t checksum;
  uint16_t id;
  uint16_t seq;
} t_icmp_echo;
_Static_assert(sizeof(t_icmp_echo) == ICMP_ECHO_HDRLEN, "ICMP echo header must be 8 bytes");

size_t icmp_echo_build(unsigned char *buf, size_t bufsz, uint16_t ident, uint16_t seq,
                       const void *payload, size_t paylen) {
  if (bufsz < ICMP_ECHO_HDRLEN || bufsz - ICMP_ECHO_HDRLEN < paylen) {
    return 0;
  }
  t_icmp_echo hdr = {
      .type = ICMP_ECHO,
      .code = 0,
      .checksum = 0,
      .id = htons(ident),
      .seq = htons(seq),
  };
  memcpy(buf, &hdr, sizeof hdr);
  if (paylen > 0) {
    memcpy(buf + ICMP_ECHO_HDRLEN, payload, paylen);
  }
  return ICMP_ECHO_HDRLEN + paylen;
}
