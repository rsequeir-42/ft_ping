/*
  ft_ping - unit tests for the ICMP module.

  Pure and deterministic: fixed inputs yield fixed bytes, so we assert the packet
  byte for byte and compute RTTs from fixed timevals. No socket, no clock.
*/

#include <criterion/criterion.h>
#include <stdint.h>
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
