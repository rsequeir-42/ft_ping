/*
    ft_ping - command-line parsing.

    options_parse() wraps GNU argp. It is reentrant (it resets *out to the
    defaults first), keeps no global state (the record travels through
    argp's input pointer), and never calls exit(): it returns a status code
    that main() turns into an exit decision.
*/

#ifndef OPTIONS_H
#define OPTIONS_H

#include "ft_ping.h"

/* -- Option flag bits, mirrored from inetutils-2.0 -- */

#define OPT_FLOOD 0x001
#define OPT_INTERVAL 0x002
#define OPT_NUMERIC 0x004
#define OPT_QUIET 0x008
#define OPT_RROUTE 0x010
#define OPT_VERBOSE 0x020
#define OPT_IPTIMESTAMP 0x040
#define OPT_DEBUG 0x080
#define OPT_IGNORE_ROUTING 0x100

/*
    Parse argc/argv into *out. Resets *out to the defaults before parsing.
    Returns 0 on success, or EX_USAGE (64) on a command-line error. Does
    not exit and does not touch the network.
*/
int options_parse(int argc, char **argv, t_options *out);

/* Print the help list on stdout */
void options_help(const char *prog);

/* Print the short usage message on stdout */
void options_usage(const char *prog);

#endif /* OPTIONS_H */
