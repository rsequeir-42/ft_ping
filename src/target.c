/*
  ft_ping - target resolution (see target.h).
*/

#include "target.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

#include "ft_ping.h"

void target_from_addrinfo(const char *host, const struct addrinfo *ai, t_target *out) {
  memcpy(&out->addr, ai->ai_addr, sizeof(out->addr));
  (void)snprintf(out->name, sizeof(out->name), "%s", ai->ai_canonname ? ai->ai_canonname : host);
  (void)inet_ntop(AF_INET, &out->addr.sin_addr, out->presentation,
                  (socklen_t)sizeof(out->presentation));
}

int target_resolve(const char *host, t_target *out) {
  struct addrinfo hints;
  struct addrinfo *res = NULL;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;     /* IPv4 only -> sockaddr_in throughout */
  hints.ai_flags = AI_CANONNAME; /* canonical name for the PING header */
  if (getaddrinfo(host, NULL, &hints, &res) != 0) {
    return 1; /* every failure is flattened into "unknown host" */
  }
  target_from_addrinfo(host, res, out);
  freeaddrinfo(res); /* once, never on failure */
  return 0;
}

int target_format_header(const t_target *t, size_t datalen, char *buf, size_t bufsz) {
  return snprintf(buf, bufsz, "PING %s (%s): %zu data bytes", t->name, t->presentation, datalen);
}
