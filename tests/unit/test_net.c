/*
  ft_ping - unit tests for the ICMP socket module.

  The pure helpers (icmp_ident, net_socket_error) are deterministic everywhere.
  net_open depends on privilege, so it is checked by CONTRACT. net_send/net_recv
  are protocol-agnostic transport, exercised over an unprivileged UDP loopback.
*/

#include <arpa/inet.h>
#include <criterion/criterion.h>
#include <errno.h>
#include <netinet/in.h>
#include <string.h>
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

/* Bind a UDP socket to an ephemeral port on 127.0.0.1; return fd, write addr. */
static int udp_bind_local(struct sockaddr_in *addr) {
  int fd = socket(AF_INET, SOCK_DGRAM, 0);
  cr_assert_geq(fd, 0);
  memset(addr, 0, sizeof *addr);
  addr->sin_family = AF_INET;
  addr->sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  addr->sin_port = 0;
  cr_assert_eq(bind(fd, (const struct sockaddr *)addr, sizeof *addr), 0);
  socklen_t sl = sizeof *addr;
  cr_assert_eq(getsockname(fd, (struct sockaddr *)addr, &sl), 0);
  return fd;
}

Test(net, send_then_recv_roundtrips_bytes) {
  struct sockaddr_in raddr;
  int rx = udp_bind_local(&raddr);
  int tx = socket(AF_INET, SOCK_DGRAM, 0);
  cr_assert_geq(tx, 0);

  const unsigned char msg[] = {0xDE, 0xAD, 0xBE, 0xEF, 0x2A};
  cr_assert_eq(net_send(tx, msg, sizeof msg, &raddr), (ssize_t)sizeof msg);

  unsigned char buf[64] = {0};
  ssize_t n = net_recv(rx, buf, sizeof buf, 1000);
  cr_assert_eq(n, (ssize_t)sizeof msg);
  cr_assert_arr_eq(buf, msg, sizeof msg);

  close(tx);
  close(rx);
}

Test(net, recv_times_out_when_silent) {
  struct sockaddr_in raddr;
  int rx = udp_bind_local(&raddr);

  unsigned char buf[64];
  ssize_t n = net_recv(rx, buf, sizeof buf, 50); /* nothing sent */
  cr_assert_eq(n, 0);

  close(rx);
}
