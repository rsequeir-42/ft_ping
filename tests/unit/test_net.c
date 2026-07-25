/*
  ft_ping - unit tests for the ICMP socket module.

  The pure helpers (icmp_ident, net_socket_error) are deterministic everywhere.
  net_open depends on privilege, so it is checked by CONTRACT (either a valid fd
  with a known socktype, or -1 with a privilege errno) so it passes with or
  without CAP_NET_RAW.
*/

#include <criterion/criterion.h>
#include <errno.h>
#include <sys/socket.h>
#include <unistd.h>

#include "net.h"

Test(net, ident_truncates_to_16_bits) {
  cr_assert_eq(icmp_ident(0), 0);
  cr_assert_eq(icmp_ident(0x1234), 0x1234);
  cr_assert_eq(icmp_ident(0x12345), 0x2345);
  cr_assert_eq(icmp_ident(0xFFFF), 0xFFFF);
}

Test(net, socket_error_privilege) {
  cr_assert_str_eq(net_socket_error(EPERM), "Lacking privilege for icmp socket.");
  cr_assert_str_eq(net_socket_error(EACCES), "Lacking privilege for icmp socket.");
  cr_assert_str_eq(net_socket_error(EPROTONOSUPPORT), "Lacking privilege for icmp socket.");
}

Test(net, socket_error_other_is_null) {
  cr_assert_null(net_socket_error(ENOMEM));
  cr_assert_null(net_socket_error(EMFILE));
}

Test(net, open_honours_its_contract) {
  int st = -1;
  int fd = net_open(&st);
  if (fd < 0) {
    cr_assert(errno == EPERM || errno == EACCES, "unexpected errno %d", errno);
  } else {
    cr_assert(st == SOCK_RAW || st == SOCK_DGRAM);
    close(fd);
  }
}
