/*
  ft_ping - command-line parsing on top of GNU argp.

  The option table mirrors inetutils-2.0 (ping/ping.c) so that --help and
  --usage read identically. Parsing is reentrant and exit-free: the parsed
  record travels through argp's input pointer (no globals), and the only
  result handed back to main() is a status code.

  Every option is wired up here: the flag bits, the request types, the
  help/usage/version actions, and the reading, validation and storage of
  the options that take an argument (numbers, the interval, the pattern).
  A bad value prints a diagnostic, records the exit status, and stops the
  parse without ever calling exit().
*/

#include "options.h"

#include <argp.h>
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sysexits.h>

#include "error.h"
#include "ft_ping.h"

/* Keys for long options that have no short counterpart. */
enum {
  ARG_ECHO = 256,
  ARG_ADDRESS,
  ARG_TIMESTAMP,
  ARG_ROUTERDISCOVERY,
  ARG_TTL,
  ARG_IPTIMESTAMP,
  ARG_USAGE
};

const char *argp_program_bug_address = "<https://github.com/rsequeir-42/ft_ping/issues>";

static const char args_doc[] = "HOST ...";
static const char doc[] = "Send ICMP ECHO_REQUEST packets to network hosts."
                          "\vOptions marked with (root only) are available "
                          "only to superuser.";

/*
  Option table. Group numbers and help strings are kept identical to
  inetutils-2.0. The help/usage/version entries are declared here (group
  -1) because ft_ping parses with ARGP_NO_HELP and therefore handles them
  itself rather than letting argp inject its own.
*/
static struct argp_option argp_options[] = {
#define GRP 0
    {NULL, 0, NULL, 0, "Options controlling ICMP request types:", GRP},
    {"address", ARG_ADDRESS, NULL, 0, "send ICMP_ADDRESS packets (root only)", GRP + 1},
    {"echo", ARG_ECHO, NULL, 0, "send ICMP_ECHO packets (default)", GRP + 1},
    {"mask", ARG_ADDRESS, NULL, 0, "same as --address", GRP + 1},
    {"timestamp", ARG_TIMESTAMP, NULL, 0, "send ICMP_TIMESTAMP packets", GRP + 1},
    {"type", 't', "TYPE", 0, "send TYPE packets", GRP + 1},
    /* This option is not yet fully implemented, so mark it as hidden. */
    {"router", ARG_ROUTERDISCOVERY, NULL, OPTION_HIDDEN,
     "send ICMP_ROUTERDISCOVERY packets (root only)", GRP + 1},
#undef GRP
#define GRP 10
    {NULL, 0, NULL, 0, "Options valid for all request types:", GRP},
    {"count", 'c', "NUMBER", 0, "stop after sending NUMBER packets", GRP + 1},
    {"debug", 'd', NULL, 0, "set the SO_DEBUG option", GRP + 1},
    {"interval", 'i', "NUMBER", 0, "wait NUMBER seconds between sending each packet", GRP + 1},
    {"numeric", 'n', NULL, 0, "do not resolve host addresses", GRP + 1},
    {"ignore-routing", 'r', NULL, 0, "send directly to a host on an attached network", GRP + 1},
    {"tos", 'T', "NUM", 0, "set type of service (TOS) to NUM", GRP + 1},
    {"ttl", ARG_TTL, "N", 0, "specify N as time-to-live", GRP + 1},
    {"verbose", 'v', NULL, 0, "verbose output", GRP + 1},
    {"timeout", 'w', "N", 0, "stop after N seconds", GRP + 1},
    {"linger", 'W', "N", 0, "number of seconds to wait for response", GRP + 1},
#undef GRP
#define GRP 20
    {NULL, 0, NULL, 0, "Options valid for --echo requests:", GRP},
    {"flood", 'f', NULL, 0, "flood ping (root only)", GRP + 1},
    {"preload", 'l', "NUMBER", 0,
     "send NUMBER packets as fast as possible before falling into normal mode of behavior (root "
     "only)",
     GRP + 1},
    {"pattern", 'p', "PATTERN", 0, "fill ICMP packet with given pattern (hex)", GRP + 1},
    {"quiet", 'q', NULL, 0, "quiet output", GRP + 1},
    {"route", 'R', NULL, 0, "record route", GRP + 1},
    {"ip-timestamp", ARG_IPTIMESTAMP, "FLAG", 0,
     "IP timestamp of type FLAG, which is one of \"tsonly\" and \"tsaddr\"", GRP + 1},
    {"size", 's', "NUMBER", 0, "send NUMBER data octets", GRP + 1},
#undef GRP
    {"help", '?', NULL, 0, "give this help list", -1},
    {"usage", ARG_USAGE, NULL, 0, "give a short usage message", -1},
    {"version", 'V', NULL, 0, "print program version", -1},
    {NULL, 0, NULL, 0, NULL, 0}};

/* Mark a value error (exit status 1) once its diagnostic has been printed. */
static int value_error(t_options *out) {
  out->status = 1;
  return 1;
}

/*
  Map a request-type name onto out->type. echo / timestamp / address (with
  its "mask" alias) are the only supported types; anything else is rejected
  the way inetutils does.
*/
static int decode_type(t_options *out, const char *prog, const char *name) {
  if (strcasecmp(name, "echo") == 0) {
    out->type = PING_ECHO;
  } else if (strcasecmp(name, "timestamp") == 0) {
    out->type = PING_TIMESTAMP;
  } else if (strcasecmp(name, "address") == 0 || strcasecmp(name, "mask") == 0) {
    out->type = PING_ADDRESS;
  } else {
    error_value(prog, "unsupported packet type: %s", name);
    return value_error(out);
  }
  return 0;
}

/*
  Convert and validate a numeric option argument -- our exit-free, robust
  take on inetutils' ping_cvt_number. strtoul with base 0 (so 0x.. and 0..
  are accepted). Rejects a leading sign, trailing junk, overflow, a value
  above maxval (0 means no cap), and zero when allow_zero is false; on
  success stores the value through *value.
*/
static int parse_number(t_options *out, const char *prog, const char *arg, size_t maxval,
                        int allow_zero, size_t *value) {
  unsigned long n;
  char *end;

  if (arg[0] == '-') {
    error_value(prog, "invalid value (`%s' near `%s')", arg, arg);
    return value_error(out);
  }
  errno = 0;
  n = strtoul(arg, &end, 0);
  if (*end != '\0') {
    error_value(prog, "invalid value (`%s' near `%s')", arg, end);
    return value_error(out);
  }
  if (errno == ERANGE) {
    error_value(prog, "option value too big: %s", arg);
    return value_error(out);
  }
  if (n == 0 && !allow_zero) {
    error_value(prog, "option value too small: %s", arg);
    return value_error(out);
  }
  if (maxval != 0 && n > maxval) {
    error_value(prog, "option value too big: %s", arg);
    return value_error(out);
  }
  *value = n;
  return 0;
}

/*
  Validate -i: a decimal number of seconds (via strtod, hence
  locale-aware), stored as milliseconds. Rejects a non-numeric tail, a
  negative value and overflow. The 200 ms non-root floor is deferred to the
  network stage (DEFERRED.md, DD-011). Sets OPT_INTERVAL.
*/
static int parse_interval(t_options *out, const char *prog, const char *arg) {
  char *end;
  double v;

  errno = 0;
  v = strtod(arg, &end);
  if (*end != '\0') {
    error_value(prog, "invalid value (`%s' near `%s')", arg, end);
    return value_error(out);
  }
  if (v < 0) {
    error_value(prog, "invalid value (`%s' near `%s')", arg, arg);
    return value_error(out);
  }
  if (errno == ERANGE || v > (double)SIZE_MAX / 1000.0) {
    error_value(prog, "option value too big: %s", arg);
    return value_error(out);
  }
  out->interval = (size_t)(v * 1000.0);
  out->flags |= OPT_INTERVAL;
  return 0;
}

/*
  Validate -l (preload): strtoul base 0, rejecting trailing junk, overflow,
  or a value above INT_MAX (which also catches a negative, since strtoul
  wraps it). inetutils' own "invalid preload value" wording is kept.
*/
static int parse_preload(t_options *out, const char *prog, const char *arg) {
  unsigned long n;
  char *end;

  errno = 0;
  n = strtoul(arg, &end, 0);
  if (*end != '\0' || errno == ERANGE || n > (unsigned long)INT_MAX) {
    error_value(prog, "invalid preload value (%s)", arg);
    return value_error(out);
  }
  out->preload = n;
  return 0;
}

/* Hex value of c (0-15), or -1 if c is not a hexadecimal digit. */
static int hex_digit(char c) {
  if (c >= '0' && c <= '9') {
    return c - '0';
  }
  if (c >= 'a' && c <= 'f') {
    return c - 'a' + 10;
  }
  if (c >= 'A' && c <= 'F') {
    return c - 'A' + 10;
  }
  return -1;
}

/*
  Parse -p: a hex string filling at most FT_PING_MAX_PATTERN bytes, one or
  two hex digits per byte. A non-hex group is rejected; unlike inetutils, an
  input longer than the limit is rejected rather than silently truncated.
*/
static int decode_pattern(t_options *out, const char *prog, const char *text) {
  int i;

  for (i = 0; *text != '\0' && i < FT_PING_MAX_PATTERN; i++) {
    int hi = hex_digit(text[0]);
    int lo;

    if (hi < 0) {
      error_value(prog, "error in pattern near %s", text);
      return value_error(out);
    }
    lo = hex_digit(text[1]);
    if (lo < 0) {
      out->pattern[i] = (unsigned char)hi;
      text += 1;
    } else {
      out->pattern[i] = (unsigned char)((hi << 4) | lo);
      text += 2;
    }
  }
  if (*text != '\0') {
    error_value(prog, "pattern too long (max %d bytes)", FT_PING_MAX_PATTERN);
    return value_error(out);
  }
  out->pattern_len = i;
  return 0;
}

/*
  Validate --ip-timestamp (tsonly or tsaddr). The exact sub-option is not
  stored yet -- that belongs to the network stage; here we only accept the
  flag. Sets OPT_IPTIMESTAMP.
*/
static int decode_ip_timestamp(t_options *out, const char *prog, const char *arg) {
  if (strcasecmp(arg, "tsonly") != 0 && strcasecmp(arg, "tsaddr") != 0) {
    error_value(prog, "unsupported timestamp type: %s", arg);
    return value_error(out);
  }
  out->flags |= OPT_IPTIMESTAMP;
  return 0;
}

/*
  Read, validate and store an option that takes an argument. Returns 0 on
  success, non-zero (out->status set, a diagnostic printed) on a bad value.
  Kept apart from parse_opt so that each switch stays simple.
*/
static int parse_value_option(t_options *out, const char *prog, int key, const char *arg) {
  size_t v;

  switch (key) {
    case 't': /* --type */
      return decode_type(out, prog, arg);
    case ARG_ROUTERDISCOVERY: /* --router (hidden, unsupported) */
      return decode_type(out, prog, "router");
    case 'c': /* --count */
      return parse_number(out, prog, arg, 0, 1, &out->count);
    case 's': /* --size */
      return parse_number(out, prog, arg, FT_PING_MAX_DATALEN, 1, &out->data_length);
    case 'i': /* --interval */
      return parse_interval(out, prog, arg);
    case 'l': /* --preload */
      return parse_preload(out, prog, arg);
    case 'p': /* --pattern */
      return decode_pattern(out, prog, arg);
    case ARG_IPTIMESTAMP: /* --ip-timestamp */
      return decode_ip_timestamp(out, prog, arg);
    case 'T': /* --tos */
      if (parse_number(out, prog, arg, 255, 1, &v)) {
        return 1;
      }
      out->tos = (int)v;
      return 0;
    case ARG_TTL: /* --ttl */
      if (parse_number(out, prog, arg, 255, 0, &v)) {
        return 1;
      }
      out->ttl = (int)v;
      return 0;
    case 'w': /* --timeout */
      if (parse_number(out, prog, arg, (size_t)INT_MAX, 0, &v)) {
        return 1;
      }
      out->timeout = (int)v;
      return 0;
    case 'W': /* --linger */
      if (parse_number(out, prog, arg, (size_t)INT_MAX, 0, &v)) {
        return 1;
      }
      out->linger = (int)v;
      return 0;
    default:
      return 0;
  }
}

/* cppcheck-suppress constParameterCallback */
/* NOLINTNEXTLINE(readability-non-const-parameter): argp fixes this signature. */
static error_t parse_opt(int key, char *arg, struct argp_state *state) {
  t_options *out = state->input;

  switch (key) {
    /* -- Flags without an argument -- */
    case 'v':
      out->flags |= OPT_VERBOSE;
      break;
    case 'q':
      out->flags |= OPT_QUIET;
      break;
    case 'n':
      out->flags |= OPT_NUMERIC;
      break;
    case 'd':
      out->flags |= OPT_DEBUG;
      break;
    case 'r':
      out->flags |= OPT_IGNORE_ROUTING;
      break;
    case 'R':
      out->flags |= OPT_RROUTE;
      break;
    case 'f':
      out->flags |= OPT_FLOOD;
      break;

    /* -- Request types -- */
    case ARG_ECHO:
      out->type = PING_ECHO;
      break;
    case ARG_TIMESTAMP:
      out->type = PING_TIMESTAMP;
      break;
    case ARG_ADDRESS:
      out->type = PING_ADDRESS;
      break;

    /* -- Help, usage and version: record the action, let main() print -- */
    case '?':
      out->action = ACT_HELP;
      break;
    case ARG_USAGE:
      out->action = ACT_USAGE;
      break;
    case 'V':
      out->action = ACT_VERSION;
      break;

    /* -- Options taking an argument: read, validate, store (see above) -- */
    case 't':
    case ARG_ROUTERDISCOVERY:
    case 'c':
    case 's':
    case 'i':
    case 'l':
    case 'p':
    case ARG_IPTIMESTAMP:
    case 'T':
    case ARG_TTL:
    case 'w':
    case 'W':
      if (parse_value_option(out, state->name, key, arg)) {
        return out->status;
      }
      break;

    /* -- Operand: a slice of argv, no copy -- */
    case ARGP_KEY_ARG:
      if (out->n_hosts == 0) {
        out->hosts = &state->argv[state->next - 1];
      }
      out->n_hosts++;
      break;

    /* -- No operand given -- */
    case ARGP_KEY_NO_ARGS:
      /* Only the normal ping job needs a host. */
      if (out->action == ACT_PING) {
        error_report(state->name, "missing host operand");
        out->status = EX_USAGE;
        return out->status;
      }
      break;

    default:
      return ARGP_ERR_UNKNOWN;
  }

  return 0;
}

static struct argp argp = {argp_options, parse_opt, args_doc, doc, NULL, NULL, NULL};

/* Reset *out to the inetutils defaults before a parse. */
static void options_reset(t_options *out) {
  memset(out, 0, sizeof(*out));
  out->action = ACT_PING;
  out->type = PING_ECHO;
  out->count = FT_PING_DEFAULT_COUNT;
  out->data_length = FT_PING_DEFAULT_DATALEN;
  out->interval = FT_PING_DEFAULT_INTERVAL;
  out->linger = FT_PING_DEFAULT_LINGER;
  out->tos = FT_PING_DEFAULT_TOS;
  out->ttl = FT_PING_DEFAULT_TTL;
  out->timeout = FT_PING_DEFAULT_TIMEOUT;
}

int options_parse(int argc, char **argv, t_options *out) {
  error_t rc;

  options_reset(out);

  /*
    ARGP_NO_EXIT    - return errors instead of calling exit().
    ARGP_NO_HELP    - we own --help / --usage / --version.
  */
  rc = argp_parse(&argp, argc, argv, ARGP_NO_EXIT | ARGP_NO_HELP, NULL, out);
  if (out->status != 0) {
    return out->status;
  }
  if (rc != 0) {
    return EX_USAGE;
  }

  return 0;
}

/*
  Render the help list on stdout, forcing the short program name. The
  caller decides when to exit (ARGP_HELP_EXIT_OK is stripped on purpose).
*/
void options_help(const char *prog) {
  argp_help(&argp, stdout, ARGP_HELP_STD_HELP & ~ARGP_HELP_EXIT_OK, (char *)prog);
}

/* Render the short usage message on stdout, forcing the short name. */
void options_usage(const char *prog) {
  argp_help(&argp, stdout, ARGP_HELP_USAGE, (char *)prog);
}
