/*
  ft_ping - ICMP Echo Request serialization, reply parsing and round-trip timing
  (see icmp.h).
*/

#include "icmp.h"

#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>

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

/* Write the header (checksum field 0) at buf[0..7] and the checksum over the
   header plus paylen payload bytes. The payload must already be in place. */
static void icmp_echo_finalize(unsigned char *buf, uint16_t ident, uint16_t seq, size_t paylen) {
  t_icmp_echo hdr = {
      .type = ICMP_ECHO,
      .code = 0,
      .checksum = 0,
      .id = htons(ident),
      .seq = htons(seq),
  };
  memcpy(buf, &hdr, sizeof hdr);
  uint16_t cksum = htons(checksum(buf, ICMP_ECHO_HDRLEN + paylen));
  memcpy(buf + offsetof(t_icmp_echo, checksum), &cksum, sizeof cksum);
}

size_t icmp_echo_build(unsigned char *buf, size_t bufsz, uint16_t ident, uint16_t seq,
                       const void *payload, size_t paylen) {
  if (bufsz < ICMP_ECHO_HDRLEN || bufsz - ICMP_ECHO_HDRLEN < paylen) {
    return 0;
  }
  if (paylen > 0) {
    memcpy(buf + ICMP_ECHO_HDRLEN, payload, paylen);
  }
  icmp_echo_finalize(buf, ident, seq, paylen);
  return ICMP_ECHO_HDRLEN + paylen;
}

size_t icmp_echo_assemble(unsigned char *buf, size_t bufsz, uint16_t ident, uint16_t seq,
                          const struct timeval *tsend, size_t datalen) {
  if (bufsz < ICMP_ECHO_HDRLEN || bufsz - ICMP_ECHO_HDRLEN < datalen) {
    return 0;
  }
  unsigned char *payload = buf + ICMP_ECHO_HDRLEN;
  size_t off = 0;
  if (datalen >= sizeof(struct timeval)) {
    memcpy(payload, tsend, sizeof *tsend);
    off = sizeof *tsend;
  }
  for (size_t i = off; i < datalen; i++) {
    payload[i] = (unsigned char)(i - off);
  }
  icmp_echo_finalize(buf, ident, seq, datalen);
  return ICMP_ECHO_HDRLEN + datalen;
}

int icmp_parse_reply(const unsigned char *buf, size_t len, int socktype, uint16_t ident,
                     t_reply *out) {
  size_t off = 0;
  int ttl = -1;

  if (socktype == SOCK_RAW) {
    struct ip iph;
    if (len < sizeof iph) {
      return -1;
    }
    memcpy(&iph, buf, sizeof iph);
    off = (size_t)iph.ip_hl * 4;
    ttl = iph.ip_ttl;
  }
  if (len < off + ICMP_ECHO_HDRLEN) {
    return -1;
  }
  size_t icmplen = len - off;
  if (checksum(buf + off, icmplen) != 0) {
    return -1;
  }
  t_icmp_echo hdr;
  memcpy(&hdr, buf + off, sizeof hdr);
  if (hdr.type != ICMP_ECHOREPLY) {
    return -1;
  }
  uint16_t id = ntohs(hdr.id);
  if (socktype == SOCK_RAW && id != ident) {
    return -1;
  }
  out->type = hdr.type;
  out->ident = id;
  out->seq = ntohs(hdr.seq);
  out->ttl = ttl;
  out->datalen = icmplen - ICMP_ECHO_HDRLEN;
  out->have_ts = 0;
  if (out->datalen >= sizeof(struct timeval)) {
    memcpy(&out->tsend, buf + off + ICMP_ECHO_HDRLEN, sizeof out->tsend);
    out->have_ts = 1;
  }
  return 0;
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
