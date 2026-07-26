/*
  ft_ping - ICMP Echo Request serialization, reply parsing and round-trip timing.

  Pure functions: no I/O, no clock, no state. icmp_echo_build/assemble write an
  Echo Request into a caller buffer; icmp_parse_reply validates a received reply;
  icmp_rtt_ms turns two timestamps into a round-trip time in milliseconds.
*/

#ifndef ICMP_H
#define ICMP_H

#include <stddef.h>
#include <stdint.h>
#include <sys/time.h>

#define ICMP_ECHO_HDRLEN 8 /* type + code + checksum + id + seq */

/* A parsed ICMP echo reply. POD: filled by icmp_parse_reply on success. */
typedef struct s_reply {
  uint8_t type;         /* ICMP type (0 = echo reply) */
  uint16_t ident;       /* identifier read from the reply */
  uint16_t seq;         /* sequence number */
  int ttl;              /* IP TTL (RAW), or -1 when unavailable (DGRAM) */
  size_t datalen;       /* ICMP payload length */
  int have_ts;          /* 1 if a send timestamp was recovered */
  struct timeval tsend; /* send timestamp echoed back in the payload */
} t_reply;

/* Serialize an ICMP Echo Request into buf: type=8, code=0, id/seq in network
   byte order, checksum left at 0 (computed later), then paylen bytes of payload
   after the header. Returns the total length written (ICMP_ECHO_HDRLEN + paylen),
   or 0 if bufsz is too small. */
size_t icmp_echo_build(unsigned char *buf, size_t bufsz, uint16_t ident, uint16_t seq,
                       const void *payload, size_t paylen);

/* Assemble a complete outgoing Echo Request: the send timestamp at payload
   offset 0 (when datalen >= sizeof(struct timeval)), then the default pattern,
   then the header and checksum. tsend is a parameter (the clock stays in the
   shell). Returns the total length, or 0 if bufsz is too small. */
size_t icmp_echo_assemble(unsigned char *buf, size_t bufsz, uint16_t ident, uint16_t seq,
                          const struct timeval *tsend, size_t datalen);

/* Parse one received datagram. socktype governs: SOCK_RAW skips the IP header
   (ip_hl * 4) and filters by id; SOCK_DGRAM reads from offset 0 and does not
   filter by id (the kernel rewrote it). Returns 0 on an echo reply that is ours
   (out filled), -1 otherwise. Pure: reads by memcpy, never a pointer cast. */
int icmp_parse_reply(const unsigned char *buf, size_t len, int socktype, uint16_t ident,
                     t_reply *out);

/* Round-trip time in milliseconds between the send and receive timestamps. */
double icmp_rtt_ms(const struct timeval *sent, const struct timeval *recv);

#endif /* ICMP_H */
