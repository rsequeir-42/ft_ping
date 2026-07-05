/*
  ft_ping - unit tests for target resolution.

  AI_NUMERICHOST yields a real addrinfo for a literal IP without any DNS, so the
  pure core (address selection, header formatting) is exercised offline. A literal
  IP also drives target_resolve() itself network-free: getaddrinfo resolves a
  dotted-quad without a DNS lookup, so its success path is covered too.
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

/* target_resolve() end to end on a literal IP: no DNS, so it stays deterministic
   and offline while covering the effectful boundary's success path. */
Test(target, resolve_literal_ip) {
  t_target t;
  cr_assert_eq(target_resolve("127.0.0.1", &t), 0);
  cr_assert_str_eq(t.name, "127.0.0.1");
  cr_assert_str_eq(t.presentation, "127.0.0.1");
  cr_assert_eq(t.addr.sin_family, AF_INET);
}
