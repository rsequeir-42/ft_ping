/*
    ft_ping - shared types, enumerations and default values.

    A recoding of inetutils-2.0 ping. This header gathers the plain-data
    option record produced by the command-line parser, the runtime shell,
    and the compile-time defaults inherited from inetutils-2.0
    (ping/ping.c, ping/ping_common.h).
*/

#ifndef FT_PING_H
#define FT_PING_H

#include <stddef.h>

/* -- Default values, mirrored from inetutils-2.0 -- */

#define FT_PING_DEFAULT_COUNT 0       /* unlimited until SIGINT */
#define FT_PING_DEFAULT_DATALEN 56    /* 64 ICMP - 8 header octets */
#define FT_PING_DEFAULT_INTERVAL 1000 /* milliseconds between packets */
#define FT_PING_DEFAULT_LINGER 10     /* seconds to wait for a reply */
#define FT_PING_DEFAULT_TOS (-1)      /* triggers only when >= 0 */
#define FT_PING_DEFAULT_TTL 0         /* 0 means "leave system default" */
#define FT_PING_DEFAULT_TIMEOUT (-1)  /* no global deadline */

#define FT_PING_MAX_PATTERN 16    /* maximal pattern length */
#define FT_PING_MAX_DATALEN 65399 /* 65535 - 60 (max IP) - 76 (max ICMP), per inetutils */

/* -- High-level action selected on the command line -- */

typedef enum e_action {
  ACT_PING = 0, /* the normal job: send echo requests */
  ACT_HELP,     /* --help / -? : print the help list */
  ACT_USAGE,    /* --usage : print the short usage message */
  ACT_VERSION   /* --version / -V : print the program version */
} t_action;

/* -- Kind of ICMP probe to emit (selected by --echo and friends) -- */

typedef enum e_ping_type {
  PING_ECHO = 0,  /* ICMP_ECHO (default) */
  PING_TIMESTAMP, /* ICMP_TIMESTAMP */
  PING_ADDRESS    /* ICMP_ADDRESS / mask */
} t_ping_type;

/*
    Parsed command-line options. Plain old data: it owns no memory. The
    host list points straight into argv, and the pattern is stored inline.
*/
typedef struct s_options {
  t_action action;  /* what to do once parsing is over */
  t_ping_type type; /* which ICMP request to send */

  unsigned int flags; /* OR-ed OPT_* bits (see options.h) */

  size_t count;          /* -c : packets to send, 0 = unlimited */
  size_t data_length;    /* -s : payload octets */
  size_t interval;       /* -i : milliseconds between packets */
  int linger;            /* -W : seconds to wait for a reply */
  int tos;               /* -T : type of service, -1 = unset */
  int ttl;               /* --ttl : time-to-live, 0 = unset */
  int timeout;           /* -w : global deadline, -1 = unset */
  unsigned long preload; /* -l : packets sent back to back */

  int pattern_len;                            /* -p : pattern length */
  unsigned char pattern[FT_PING_MAX_PATTERN]; /* -p : inline pattern */

  char **hosts;   /* operands, a slice of argv */
  size_t n_hosts; /* number of operands */

  int status; /* exit code set on a parse error (0 = success) */
} t_options;

/*
    Runtime shell. Empty on purpose for now: the network engine fills it in
    later. Kept here so the rest of the program can already refer to it.
*/
typedef struct s_ping {
  int fd; /* raw socket descriptor */
} t_ping;

#endif /* FT_PING_H */
