/*
  ft_ping - unit tests for the RFC 1071 Internet checksum.

  Pure and deterministic: fixed input yields a fixed 16-bit value, so we assert
  the exact checksum against vectors verified with scapy and the RFC.
*/

#include <criterion/criterion.h>
#include <stdint.h>

#include "checksum.h"

Test(checksum, rfc1071_vector) {
  const unsigned char v[] = {0x00, 0x01, 0xf2, 0x03, 0xf4, 0xf5, 0xf6, 0xf7};
  cr_assert_eq(checksum(v, sizeof v), 0x220d);
}
Test(checksum, icmp_header) {
  const unsigned char v[] = {0x08, 0x00, 0x00, 0x00, 0x12, 0x34, 0x00, 0x01};
  cr_assert_eq(checksum(v, sizeof v), 0xe5ca);
}
Test(checksum, icmp_with_payload) {
  const unsigned char v[] = {0x08, 0x00, 0x00, 0x00, 0x12, 0x34,
                             0x00, 0x01, 0xDE, 0xAD, 0xBE, 0xEF};
  cr_assert_eq(checksum(v, sizeof v), 0x482d);
}
Test(checksum, empty_buffer) {
  cr_assert_eq(checksum(NULL, 0), 0xffff);
}
Test(checksum, single_odd_byte_is_high_half) {
  const unsigned char v[] = {0xff};
  cr_assert_eq(checksum(v, sizeof v), 0x00ff); /* high half, not 0xff00 */
}
Test(checksum, odd_length) {
  const unsigned char v[] = {0x12, 0x34, 0x56};
  cr_assert_eq(checksum(v, sizeof v), 0x97cb);
}
Test(checksum, carry_fold) {
  const unsigned char v[] = {0xff, 0xff, 0xff, 0xff};
  cr_assert_eq(checksum(v, sizeof v), 0x0000);
}
Test(checksum, verify_recomputes_to_zero) {
  unsigned char v[12] = {0x08, 0x00, 0x00, 0x00, 0x12, 0x34, 0x00, 0x01, 0xDE, 0xAD, 0xBE, 0xEF};
  uint16_t c = checksum(v, sizeof v);
  v[2] = (unsigned char)(c >> 8);
  v[3] = (unsigned char)(c & 0xFF);
  cr_assert_eq(checksum(v, sizeof v), 0);
}
