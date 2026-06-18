/*
    ft_ping - command-line parsing on top of GNU argp.

    The option table mirrors inetutils-2.0 (ping/ping.c) so that --help and
    --usage read identically. Parsing is reentrant and exit-free: the parsed
    record travels through argp's input pointer (no globals), and the only
    result handed back to main() is a status code.

    This step wires up the flags without arguments, the request types, and
    the help/usage/version requests. Options that take an argument are
    declared here (for the help text) but their handlers are placeholders:
    reading and validating their values is the next step.
*/

#include "options.h"

#include <argp.h>
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

/* Map --echo and friends onto the request type stored in the record. */
static t_ping_type decode_type(const char *name) {
  if (strcasecmp(name, "echo") == 0) {
    return PING_ECHO;
  }
  if (strcasecmp(name, "timestamp") == 0) {
    return PING_TIMESTAMP;
  }
  if (strcasecmp(name, "address") == 0) {
    return PING_ADDRESS;
  }
  if (strcasecmp(name, "mask") == 0) {
    return PING_ADDRESS;
  }
  /* "router" and unknown names: validated later, with the argument. */
  return PING_ECHO;
}

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
      out->type = decode_type("echo");
      break;
    case ARG_TIMESTAMP:
      out->type = decode_type("timestamp");
      break;
    case ARG_ADDRESS:
      out->type = decode_type("address");
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

    /*
        Options taking an argument. Declared above for the help text;
        their values are read and validated in the next step. Accept the
        argument and move on without storing anything.
    */
    case 't':                 /* --type */
    case ARG_ROUTERDISCOVERY: /* --router (hidden) */
    case 'c':                 /* --count */
    case 'i':                 /* --interval */
    case 'T':                 /* --tos */
    case ARG_TTL:             /* --ttl */
    case 'w':                 /* --timeout */
    case 'W':                 /* --linger */
    case 'l':                 /* --preload */
    case 'p':                 /* --pattern */
    case 's':                 /* --size */
    case ARG_IPTIMESTAMP:     /* --ip-timestamp */
      (void)arg;
      /* argument parsing: next step */
      return 0;

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
        return EX_USAGE;
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
