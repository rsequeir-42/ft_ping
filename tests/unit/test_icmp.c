/*
  ft_ping - unit tests for the ICMP Echo Request serializer.

  Pure and deterministic: a fixed ident/seq/payload yields a fixed byte string,
  so we assert the whole packet byte for byte. No socket, no clock.
*/

#include <criterion/criterion.h>
#include <stdint.h>

#include "checksum.h"
#include "icmp.h"

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
  cr_assert_eq(icmp_echo_build(buf2, sizeof buf2, 1, 1, pay, sizeof pay), 0); /* 8+4 > 9 */
}

Test(icmp, echo_build_is_wire_checksummed) {
  unsigned char buf[12] = {0};
  const unsigned char pay[] = {0xDE, 0xAD, 0xBE, 0xEF};
  size_t n = icmp_echo_build(buf, sizeof buf, 0x1234, 0x0001, pay, sizeof pay);
  cr_assert_eq(checksum(buf, n), 0); /* a valid on-wire packet re-sums to 0 */
}
