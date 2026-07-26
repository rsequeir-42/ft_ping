/*
  ft_ping - ICMP Echo Request serialization and round-trip timing (see icmp.h).
*/

#include "icmp.h"

#include <arpa/inet.h>
#include <netinet/ip_icmp.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "checksum.h"

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
  /* Checksum last, over header (field = 0) plus payload; store big-endian. */
  uint16_t cksum = htons(checksum(buf, ICMP_ECHO_HDRLEN + paylen));
  memcpy(buf + offsetof(t_icmp_echo, checksum), &cksum, sizeof cksum);
  return ICMP_ECHO_HDRLEN + paylen;
}

/* out = recv - sent, borrowing on the microseconds (inetutils' tvsub). */
static struct timeval tv_sub(const struct timeval *recv, const struct timeval *sent) {
  struct timeval out = *recv;

  out.tv_usec -= sent->tv_usec;
  if (out.tv_usec < 0) {
    out.tv_sec -= 1;
    out.tv_usec += 1000000;
  }
  out.tv_sec -= sent->tv_sec;
  return out;
}

double icmp_rtt_ms(const struct timeval *sent, const struct timeval *recv) {
  struct timeval d = tv_sub(recv, sent);

  return ((double)d.tv_sec * 1000.0) + ((double)d.tv_usec / 1000.0);
}
