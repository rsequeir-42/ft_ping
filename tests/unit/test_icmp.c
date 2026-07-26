/*
  ft_ping - unit tests for the ICMP module.

  Pure and deterministic: fixed inputs yield fixed bytes, so we assert the packet
  byte for byte, parse hand-built replies, and compute RTTs from fixed timevals.
  No socket, no clock.
*/

#include <arpa/inet.h>
#include <criterion/criterion.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>

#include "checksum.h"
#include "icmp.h"

/* --- icmp_echo_build --- */

Test(icmp, echo_build_byte_exact) {
  unsigned char buf[12] = {0};
  const unsigned char payload[] = {0xDE, 0xAD, 0xBE, 0xEF};
  const unsigned char want[] = {0x08, 0x00, 0x48, 0x2d, 0x12, 0x34,
                                0x00, 0x01, 0xDE, 0xAD, 0xBE, 0xEF};
  size_t n = icmp_echo_build(buf, sizeof buf, 0x1234, 0x0001, payload, sizeof payload);
  cr_assert_eq(n, 12);
  cr_assert_arr_eq(buf, want, sizeof want);
}

Test(icmp, echo_build_empty_payload) {
  unsigned char buf[8] = {0};
  const unsigned char want[] = {0x08, 0x00, 0x4c, 0x08, 0xAB, 0xCD, 0x00, 0x2A};
  size_t n = icmp_echo_build(buf, sizeof buf, 0xABCD, 0x002A, NULL, 0);
  cr_assert_eq(n, 8);
  cr_assert_arr_eq(buf, want, sizeof want);
}

Test(icmp, echo_build_buffer_too_small) {
  unsigned char buf[7] = {0};
  const unsigned char pay[4] = {1, 2, 3, 4};
  cr_assert_eq(icmp_echo_build(buf, sizeof buf, 0x1234, 1, NULL, 0), 0);
  unsigned char buf2[9] = {0};
  cr_assert_eq(icmp_echo_build(buf2, sizeof buf2, 1, 1, pay, sizeof pay), 0);
}

Test(icmp, echo_build_is_wire_checksummed) {
  unsigned char buf[12] = {0};
  const unsigned char pay[] = {0xDE, 0xAD, 0xBE, 0xEF};
  size_t n = icmp_echo_build(buf, sizeof buf, 0x1234, 0x0001, pay, sizeof pay);
  cr_assert_eq(checksum(buf, n), 0);
}

/* --- icmp_parse_reply --- */

/* Build an ICMP message (given type) with a valid checksum into buf. */
static size_t build_reply(unsigned char *buf, uint8_t type, uint16_t id, uint16_t seq,
                          const unsigned char *pay, size_t paylen) {
  uint16_t nid = htons(id);
  uint16_t nseq = htons(seq);

  buf[0] = type;
  buf[1] = 0;
  buf[2] = 0;
  buf[3] = 0;
  memcpy(buf + 4, &nid, 2);
  memcpy(buf + 6, &nseq, 2);
  if (paylen > 0) {
    memcpy(buf + 8, pay, paylen);
  }
  uint16_t ck = htons(checksum(buf, 8 + paylen));
  memcpy(buf + 2, &ck, 2);
  return 8 + paylen;
}

/* Prepend a minimal 20-byte IPv4 header (ihl=5, given ttl) before an ICMP msg. */
static void prepend_ip(unsigned char *buf, uint8_t ttl) {
  memset(buf, 0, 20);
  buf[0] = 0x45; /* version 4, ihl 5 */
  buf[8] = ttl;  /* ip_ttl */
}

Test(icmp, parse_raw_reply_ok) {
  unsigned char icmp[8 + 16];
  struct timeval ts = {.tv_sec = 42, .tv_usec = 43};
  size_t ilen = build_reply(icmp, 0, 0x1234, 0x0009, (const unsigned char *)&ts, sizeof ts);
  unsigned char pkt[20 + 8 + 16];
  prepend_ip(pkt, 64);
  memcpy(pkt + 20, icmp, ilen);

  t_reply out;
  int rc = icmp_parse_reply(pkt, 20 + ilen, SOCK_RAW, 0x1234, &out);
  cr_assert_eq(rc, 0);
  cr_assert_eq(out.type, 0);
  cr_assert_eq(out.ident, 0x1234);
  cr_assert_eq(out.seq, 0x0009);
  cr_assert_eq(out.ttl, 64);
  cr_assert_eq(out.datalen, 16);
  cr_assert_eq(out.have_ts, 1);
  cr_assert_eq(memcmp(&out.tsend, &ts, sizeof ts), 0);
}

Test(icmp, parse_dgram_reply_ignores_id) {
  unsigned char icmp[8 + 16];
  struct timeval ts = {.tv_sec = 1, .tv_usec = 2};
  size_t ilen = build_reply(icmp, 0, 0x0002, 0x0003, (const unsigned char *)&ts, sizeof ts);

  t_reply out;
  /* ident 0x9999 != reply id 0x0002, but DGRAM must not filter by id */
  int rc = icmp_parse_reply(icmp, ilen, SOCK_DGRAM, 0x9999, &out);
  cr_assert_eq(rc, 0);
  cr_assert_eq(out.ttl, -1); /* no IP header in DGRAM */
  cr_assert_eq(out.seq, 0x0003);
  cr_assert_eq(out.have_ts, 1);
}

Test(icmp, parse_rejects_too_short) {
  unsigned char pkt[5] = {0x45, 0, 0, 0, 0};
  t_reply out;
  cr_assert_eq(icmp_parse_reply(pkt, sizeof pkt, SOCK_RAW, 1, &out), -1);
  cr_assert_eq(icmp_parse_reply(pkt, 3, SOCK_DGRAM, 1, &out), -1);
}

Test(icmp, parse_rejects_bad_checksum) {
  unsigned char icmp[8 + 4];
  const unsigned char pay[] = {1, 2, 3, 4};
  size_t ilen = build_reply(icmp, 0, 0x1234, 1, pay, sizeof pay);
  icmp[9] ^= 0xFF; /* corrupt the payload after the checksum was set */
  t_reply out;
  cr_assert_eq(icmp_parse_reply(icmp, ilen, SOCK_DGRAM, 0x1234, &out), -1);
}

Test(icmp, parse_rejects_wrong_type) {
  unsigned char icmp[8 + 4];
  const unsigned char pay[] = {1, 2, 3, 4};
  size_t ilen = build_reply(icmp, 3, 0x1234, 1, pay, sizeof pay); /* dest unreachable */
  t_reply out;
  cr_assert_eq(icmp_parse_reply(icmp, ilen, SOCK_DGRAM, 0x1234, &out), -1);
}

Test(icmp, parse_rejects_echo_request) {
  unsigned char icmp[8 + 4];
  const unsigned char pay[] = {1, 2, 3, 4};
  size_t ilen = build_reply(icmp, 8, 0x1234, 1, pay, sizeof pay); /* our own request */
  t_reply out;
  cr_assert_eq(icmp_parse_reply(icmp, ilen, SOCK_DGRAM, 0x1234, &out), -1);
}

Test(icmp, parse_rejects_foreign_id_on_raw) {
  unsigned char icmp[8 + 4];
  const unsigned char pay[] = {1, 2, 3, 4};
  size_t ilen = build_reply(icmp, 0, 0x5678, 1, pay, sizeof pay);
  unsigned char pkt[20 + 8 + 4];
  prepend_ip(pkt, 64);
  memcpy(pkt + 20, icmp, ilen);
  t_reply out;
  cr_assert_eq(icmp_parse_reply(pkt, 20 + ilen, SOCK_RAW, 0x1234, &out), -1);
}

/* --- icmp_rtt_ms --- */

Test(icmp, rtt_sub_millisecond) {
  struct timeval sent = {.tv_sec = 100, .tv_usec = 0};
  struct timeval recv = {.tv_sec = 100, .tv_usec = 50};
  cr_assert_float_eq(icmp_rtt_ms(&sent, &recv), 0.050, 1e-9);
}

Test(icmp, rtt_mixed) {
  struct timeval sent = {.tv_sec = 10, .tv_usec = 500000};
  struct timeval recv = {.tv_sec = 10, .tv_usec = 512345};
  cr_assert_float_eq(icmp_rtt_ms(&sent, &recv), 12.345, 1e-6);
}

Test(icmp, rtt_borrows_across_second) {
  struct timeval sent = {.tv_sec = 100, .tv_usec = 900000};
  struct timeval recv = {.tv_sec = 101, .tv_usec = 200000};
  cr_assert_float_eq(icmp_rtt_ms(&sent, &recv), 300.0, 1e-6);
}
