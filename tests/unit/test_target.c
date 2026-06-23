/*
  ft_ping - unit tests for target resolution (pure core).

  AI_NUMERICHOST yields a real addrinfo for a literal IP without any DNS, so the
  address-selection and header-formatting logic is exercised offline.
*/

#include <criterion/criterion.h>
#include <netdb.h>
#include <string.h>
#include <sys/socket.h>

#include "ft_ping.h"
#include "target.h"

/* Build a real addrinfo for a literal IPv4 (no network), then fill *out. */
static void from_literal(const char *ip, t_target *out) {
  struct addrinfo hints;
  struct addrinfo *res = NULL;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_flags = AI_NUMERICHOST | AI_CANONNAME;
  cr_assert_eq(getaddrinfo(ip, NULL, &hints, &res), 0, "getaddrinfo(%s)", ip);
  target_from_addrinfo(ip, res, out);
  freeaddrinfo(res);
}

Test(target, from_addrinfo_loopback) {
  t_target t;
  from_literal("127.0.0.1", &t);
  cr_assert_str_eq(t.name, "127.0.0.1");
  cr_assert_str_eq(t.presentation, "127.0.0.1");
  cr_assert_eq(t.addr.sin_family, AF_INET);
}

Test(target, from_addrinfo_public_literal) {
  t_target t;
  from_literal("8.8.8.8", &t);
  cr_assert_str_eq(t.presentation, "8.8.8.8");
}

Test(target, header_default_datalen) {
  t_target t;
  char buf[128];
  from_literal("127.0.0.1", &t);
  target_format_header(&t, 56, buf, sizeof(buf));
  cr_assert_str_eq(buf, "PING 127.0.0.1 (127.0.0.1): 56 data bytes");
}

Test(target, header_reflects_s_option) {
  t_target t;
  char buf[128];
  from_literal("127.0.0.1", &t);
  target_format_header(&t, 100, buf, sizeof(buf));
  cr_assert_str_eq(buf, "PING 127.0.0.1 (127.0.0.1): 100 data bytes");
}
