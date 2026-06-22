/*
  ft_ping - target resolution.

  Turns a HOST operand into a t_target (IPv4 address + display name). The
  effectful target_resolve() is the only network I/O; the rest is pure and
  unit-tested without DNS.
*/

#ifndef TARGET_H
#define TARGET_H

#include <netdb.h>

#include "ft_ping.h"

/* Resolve HOST into *out. The ONLY network I/O. Returns 0 on success, or 1 on
   any getaddrinfo failure (the caller reports "unknown host"). */
int target_resolve(const char *host, t_target *out);

/* Fill *out from the first address of AI: copies the sockaddr, the canonical
   name (HOST as fallback) and the dotted-quad. Pure (no network). */
void target_from_addrinfo(const char *host, const struct addrinfo *ai, t_target *out);

/* Write "PING <name> (<presentation>): <datalen> data bytes" into BUF (no
   trailing newline, no -v suffix). Returns snprintf's value. Pure. */
int target_format_header(const t_target *t, size_t datalen, char *buf, size_t bufsz);

#endif /* TARGET_H */
