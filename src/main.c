/*
  ft_ping - program entry point.

  A thin humble object: set the locale, parse the command line, then act on the
  outcome. The help, usage and version requests print to stdout and exit
  successfully; a parsing error exits with its status code; a normal ping run
  resolves its targets, opens the socket, and sends one echo request per host,
  awaiting the reply. Silent for now (display lands next): the exit code reports
  whether any reply was received.
*/

#include <errno.h>
#include <locale.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include "error.h"
#include "ft_ping.h"
#include "icmp.h"
#include "net.h"
#include "options.h"
#include "target.h"

/* Largest packet we build (header + max payload) or receive (IP + ICMP +
   payload); a full IP datagram never exceeds this. */
#define PING_BUF_MAX 65536

static const char version[] = "ft_ping 1.0\n"
                              "Copyright (C) 2026 Rafael Sequeira.\n"
                              "License MIT: <https://opensource.org/licenses/MIT>.\n"
                              "This is free software: you are free to change and redistribute it.\n"
                              "There is NO WARRANTY, to the extent permitted by law.\n";

/* Short program name (argv[0] basename), used for help and diagnostics. */
static const char *short_name(const char *argv0) {
  const char *slash;

  if (argv0 == NULL || *argv0 == '\0') {
    return "ft_ping";
  }
  slash = strrchr(argv0, '/');
  return slash ? slash + 1 : argv0;
}

/* Receive until our echo reply arrives or the budget runs out, rejecting our
   own looped-back requests and other traffic and looping with the time left.
   Returns 0 (out filled, trecv captured) or -1 (timeout / error). */
static int ping_recv_reply(const t_ping *ping, int timeout_ms, t_reply *out,
                           struct timeval *trecv) {
  unsigned char buf[PING_BUF_MAX];
  struct timeval start;
  int remaining = timeout_ms;

  gettimeofday(&start, NULL);
  while (remaining > 0) {
    ssize_t n = net_recv(ping->fd, buf, sizeof buf, remaining);
    if (n > 0) {
      gettimeofday(trecv, NULL);
      if (icmp_parse_reply(buf, (size_t)n, ping->socktype, (uint16_t)ping->ident, out) == 0) {
        return 0;
      }
    } else if (n == 0 || errno != EINTR) {
      return -1; /* timeout, or a real receive error */
    }
    struct timeval now;
    gettimeofday(&now, NULL);
    long elapsed = ((now.tv_sec - start.tv_sec) * 1000) + ((now.tv_usec - start.tv_usec) / 1000);
    remaining = timeout_ms - (int)elapsed;
  }
  return -1;
}

int main(int argc, char **argv) {
  t_options options;
  const char *prog;
  int rc;

  (void)setlocale(LC_ALL, "");
  prog = short_name(argv[0]);

  rc = options_parse(argc, argv, &options);

  switch (options.action) {
    case ACT_HELP:
      options_help(prog);
      exit(EXIT_SUCCESS);
    case ACT_USAGE:
      options_usage(prog);
      exit(EXIT_SUCCESS);
    case ACT_VERSION:
      (void)fputs(version, stdout);
      exit(EXIT_SUCCESS);
    case ACT_PING:
    default:
      break;
  }

  if (rc != 0) {
    exit(rc);
  }

  /* ACT_PING with a valid command line: resolve each operand, open the socket
     once, and send one echo request per host, awaiting its reply. */
  t_ping ping = {0};
  ping.fd = -1;
  ping.ident = icmp_ident(getpid());
  int received = 0;
  for (size_t i = 0; i < options.n_hosts; i++) {
    if (target_resolve(options.hosts[i], &ping.target) != 0) {
      error_value(prog, "unknown host");
      exit(EXIT_FAILURE);
    }
    if (ping.fd < 0) {
      ping.fd = net_open(&ping.socktype);
      if (ping.fd < 0) {
        const char *m = net_socket_error(errno);
        error_value(prog, "%s", m ? m : strerror(errno));
        exit(EXIT_FAILURE);
      }
    }

    struct timeval tsend;
    unsigned char sbuf[PING_BUF_MAX];
    gettimeofday(&tsend, NULL);
    size_t len =
        icmp_echo_assemble(sbuf, sizeof sbuf, (uint16_t)ping.ident, 0, &tsend, options.data_length);
    if (net_send(ping.fd, sbuf, len, &ping.target.addr) < 0) {
      error_value(prog, "%s", strerror(errno));
      exit(EXIT_FAILURE);
    }

    t_reply reply;
    struct timeval trecv;
    if (ping_recv_reply(&ping, FT_PING_DEFAULT_LINGER * 1000, &reply, &trecv) == 0) {
      received = 1;
      if (reply.have_ts) {
        double rtt = icmp_rtt_ms(&reply.tsend, &trecv);
        (void)rtt; /* computed now; printed next sprint */
      }
    }
  }
  if (ping.fd >= 0) {
    close(ping.fd);
  }
  return received ? 0 : 1;
}
