/*
    Unit tests for the command-line parser, options_parse().

    These exercise the parser as it stands at this step: flags without an
    argument, request types, the help/usage/version actions, operand
    collection, the "--" separator and reentrance. Options that take an
    argument are accepted but their values are not stored yet, so the tests
    only check that the line is accepted and the argument is consumed.

    Conventions:
        - argv is a local array of string literals terminated by NULL; argc is
          the count without the trailing NULL. argp only reorders the pointers,
          never the strings, so literals are safe here.
        - error paths print on stderr; the redirect_all init swallows that so
          the diagnostics never reach the test output. Message contents are not
          asserted (left to a future bats suite).
*/

#include <criterion/criterion.h>
#include <criterion/new/assert.h>
#include <criterion/redirect.h>

#include "ft_ping.h"
#include "options.h"

/* Silence stdout/stderr for suites that drive the parser through errors or
   the help/usage/version actions. */
static void redirect_all(void) {
  cr_redirect_stdout();
  cr_redirect_stderr();
}

/* -- defaults -- */

Test(defaults, plain_host_yields_inetutils_defaults) {
  char *argv[] = {"ft_ping", "host", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(2, argv, &o), 0));
  cr_assert(eq(int, o.action, ACT_PING));
  cr_assert(eq(int, o.type, PING_ECHO));
  cr_assert(eq(int, (int)o.flags, 0));
  cr_assert(eq(sz, o.count, 0));
  cr_assert(eq(sz, o.data_length, 56));
  cr_assert(eq(dbl, o.interval, 1.0));
  cr_assert(eq(int, o.linger, 10));
  cr_assert(eq(int, o.tos, -1));
  cr_assert(eq(int, o.ttl, 0));
  cr_assert(eq(int, o.timeout, -1));
  cr_assert(eq(int, (int)o.preload, 0));
  cr_assert(eq(int, o.pattern_len, 0));
  cr_assert(eq(sz, o.n_hosts, 1));
  cr_assert(eq(str, o.hosts[0], "host"));
}

/* -- flags_short -- */

Test(flags_short, verbose) {
  char *argv[] = {"ft_ping", "-v", "host", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(3, argv, &o), 0));
  cr_assert(eq(int, (int)(o.flags & OPT_VERBOSE), OPT_VERBOSE));
}

Test(flags_short, quiet) {
  char *argv[] = {"ft_ping", "-q", "host", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(3, argv, &o), 0));
  cr_assert(eq(int, (int)(o.flags & OPT_QUIET), OPT_QUIET));
}

Test(flags_short, numeric) {
  char *argv[] = {"ft_ping", "-n", "host", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(3, argv, &o), 0));
  cr_assert(eq(int, (int)(o.flags & OPT_NUMERIC), OPT_NUMERIC));
}

Test(flags_short, debug) {
  char *argv[] = {"ft_ping", "-d", "host", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(3, argv, &o), 0));
  cr_assert(eq(int, (int)(o.flags & OPT_DEBUG), OPT_DEBUG));
}

Test(flags_short, ignore_routing) {
  char *argv[] = {"ft_ping", "-r", "host", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(3, argv, &o), 0));
  cr_assert(eq(int, (int)(o.flags & OPT_IGNORE_ROUTING), OPT_IGNORE_ROUTING));
}

Test(flags_short, rroute) {
  char *argv[] = {"ft_ping", "-R", "host", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(3, argv, &o), 0));
  cr_assert(eq(int, (int)(o.flags & OPT_RROUTE), OPT_RROUTE));
}

Test(flags_short, flood) {
  char *argv[] = {"ft_ping", "-f", "host", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(3, argv, &o), 0));
  cr_assert(eq(int, (int)(o.flags & OPT_FLOOD), OPT_FLOOD));
}

/* -- flags_long -- */

Test(flags_long, verbose) {
  char *argv[] = {"ft_ping", "--verbose", "host", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(3, argv, &o), 0));
  cr_assert(eq(int, (int)(o.flags & OPT_VERBOSE), OPT_VERBOSE));
}

Test(flags_long, quiet) {
  char *argv[] = {"ft_ping", "--quiet", "host", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(3, argv, &o), 0));
  cr_assert(eq(int, (int)(o.flags & OPT_QUIET), OPT_QUIET));
}

Test(flags_long, numeric) {
  char *argv[] = {"ft_ping", "--numeric", "host", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(3, argv, &o), 0));
  cr_assert(eq(int, (int)(o.flags & OPT_NUMERIC), OPT_NUMERIC));
}

Test(flags_long, debug) {
  char *argv[] = {"ft_ping", "--debug", "host", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(3, argv, &o), 0));
  cr_assert(eq(int, (int)(o.flags & OPT_DEBUG), OPT_DEBUG));
}

Test(flags_long, ignore_routing) {
  char *argv[] = {"ft_ping", "--ignore-routing", "host", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(3, argv, &o), 0));
  cr_assert(eq(int, (int)(o.flags & OPT_IGNORE_ROUTING), OPT_IGNORE_ROUTING));
}

Test(flags_long, route) {
  char *argv[] = {"ft_ping", "--route", "host", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(3, argv, &o), 0));
  cr_assert(eq(int, (int)(o.flags & OPT_RROUTE), OPT_RROUTE));
}

Test(flags_long, flood) {
  char *argv[] = {"ft_ping", "--flood", "host", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(3, argv, &o), 0));
  cr_assert(eq(int, (int)(o.flags & OPT_FLOOD), OPT_FLOOD));
}

/* -- flags_combined -- */

Test(flags_combined, clustered_short_flags) {
  char *argv[] = {"ft_ping", "-dnv", "host", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(3, argv, &o), 0));
  cr_assert(eq(int, (int)o.flags, OPT_DEBUG | OPT_NUMERIC | OPT_VERBOSE));
}

Test(flags_combined, repeated_flag_is_idempotent) {
  char *argv[] = {"ft_ping", "-vvv", "host", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(3, argv, &o), 0));
  cr_assert(eq(int, (int)o.flags, OPT_VERBOSE));
}

Test(flags_combined, separate_short_flags) {
  char *argv[] = {"ft_ping", "-v", "-q", "-n", "host", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(5, argv, &o), 0));
  cr_assert(eq(int, (int)o.flags, OPT_VERBOSE | OPT_QUIET | OPT_NUMERIC));
}

/* -- types -- */

Test(types, default_is_echo) {
  char *argv[] = {"ft_ping", "host", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(2, argv, &o), 0));
  cr_assert(eq(int, o.type, PING_ECHO));
}

Test(types, echo) {
  char *argv[] = {"ft_ping", "--echo", "host", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(3, argv, &o), 0));
  cr_assert(eq(int, o.type, PING_ECHO));
}

Test(types, timestamp) {
  char *argv[] = {"ft_ping", "--timestamp", "host", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(3, argv, &o), 0));
  cr_assert(eq(int, o.type, PING_TIMESTAMP));
}

Test(types, address) {
  char *argv[] = {"ft_ping", "--address", "host", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(3, argv, &o), 0));
  cr_assert(eq(int, o.type, PING_ADDRESS));
}

Test(types, mask_is_address) {
  char *argv[] = {"ft_ping", "--mask", "host", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(3, argv, &o), 0));
  cr_assert(eq(int, o.type, PING_ADDRESS));
}

Test(types, last_type_wins_address_then_echo) {
  char *argv[] = {"ft_ping", "--address", "--echo", "host", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(4, argv, &o), 0));
  cr_assert(eq(int, o.type, PING_ECHO));
}

Test(types, last_type_wins_echo_then_timestamp) {
  char *argv[] = {"ft_ping", "--echo", "--timestamp", "host", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(4, argv, &o), 0));
  cr_assert(eq(int, o.type, PING_TIMESTAMP));
}

/* -- actions -- */

Test(actions, help_long, .init = redirect_all) {
  char *argv[] = {"ft_ping", "--help", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(2, argv, &o), 0));
  cr_assert(eq(int, o.action, ACT_HELP));
}

Test(actions, help_short, .init = redirect_all) {
  char *argv[] = {"ft_ping", "-?", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(2, argv, &o), 0));
  cr_assert(eq(int, o.action, ACT_HELP));
}

Test(actions, usage, .init = redirect_all) {
  char *argv[] = {"ft_ping", "--usage", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(2, argv, &o), 0));
  cr_assert(eq(int, o.action, ACT_USAGE));
}

Test(actions, version_long, .init = redirect_all) {
  char *argv[] = {"ft_ping", "--version", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(2, argv, &o), 0));
  cr_assert(eq(int, o.action, ACT_VERSION));
}

Test(actions, version_short, .init = redirect_all) {
  char *argv[] = {"ft_ping", "-V", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(2, argv, &o), 0));
  cr_assert(eq(int, o.action, ACT_VERSION));
}

Test(actions, host_is_ping, .init = redirect_all) {
  char *argv[] = {"ft_ping", "host", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(2, argv, &o), 0));
  cr_assert(eq(int, o.action, ACT_PING));
}

/* -- operands -- */

Test(operands, single_host) {
  char *argv[] = {"ft_ping", "host", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(2, argv, &o), 0));
  cr_assert(eq(sz, o.n_hosts, 1));
  cr_assert(eq(str, o.hosts[0], "host"));
}

Test(operands, three_hosts) {
  char *argv[] = {"ft_ping", "h1", "h2", "h3", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(4, argv, &o), 0));
  cr_assert(eq(sz, o.n_hosts, 3));
  cr_assert(eq(str, o.hosts[0], "h1"));
  cr_assert(eq(str, o.hosts[1], "h2"));
  cr_assert(eq(str, o.hosts[2], "h3"));
}

Test(operands, flag_before_hosts) {
  char *argv[] = {"ft_ping", "-v", "h1", "h2", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(4, argv, &o), 0));
  cr_assert(eq(sz, o.n_hosts, 2));
  cr_assert(eq(int, (int)(o.flags & OPT_VERBOSE), OPT_VERBOSE));
  cr_assert(eq(str, o.hosts[0], "h1"));
  cr_assert(eq(str, o.hosts[1], "h2"));
}

Test(operands, flag_between_hosts_keeps_relative_order) {
  char *argv[] = {"ft_ping", "h1", "-v", "h2", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(4, argv, &o), 0));
  cr_assert(eq(sz, o.n_hosts, 2));
  cr_assert(eq(int, (int)(o.flags & OPT_VERBOSE), OPT_VERBOSE));
  /* argp permutes argv but preserves the relative order of operands. */
  cr_assert(eq(str, o.hosts[0], "h1"));
  cr_assert(eq(str, o.hosts[1], "h2"));
}

/* -- errors -- */

Test(errors, no_argument_at_all, .init = redirect_all) {
  char *argv[] = {"ft_ping", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(1, argv, &o), 64));
}

Test(errors, flag_without_host, .init = redirect_all) {
  char *argv[] = {"ft_ping", "-v", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(2, argv, &o), 64));
}

Test(errors, unknown_short_option, .init = redirect_all) {
  char *argv[] = {"ft_ping", "-Z", "host", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(3, argv, &o), 64));
}

Test(errors, unknown_long_option, .init = redirect_all) {
  char *argv[] = {"ft_ping", "--bogus", "host", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(3, argv, &o), 64));
}

Test(errors, help_without_host_is_ok, .init = redirect_all) {
  char *argv[] = {"ft_ping", "--help", NULL};
  t_options o;

  /* The action takes precedence: a missing host is not an error. */
  cr_assert(eq(int, options_parse(2, argv, &o), 0));
  cr_assert(eq(int, o.action, ACT_HELP));
}

Test(errors, version_without_host_is_ok, .init = redirect_all) {
  char *argv[] = {"ft_ping", "--version", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(2, argv, &o), 0));
  cr_assert(eq(int, o.action, ACT_VERSION));
}

/* -- value options -- */
/* Each option taking an argument is accepted (rc 0), its argument consumed,
   and the trailing host captured. Stored values are not checked yet. */

Test(value_options, count, .init = redirect_all) {
  char *argv[] = {"ft_ping", "-c", "5", "host", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(4, argv, &o), 0));
  cr_assert(eq(sz, o.n_hosts, 1));
  cr_assert(eq(str, o.hosts[0], "host"));
}

Test(value_options, ttl, .init = redirect_all) {
  char *argv[] = {"ft_ping", "--ttl", "64", "host", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(4, argv, &o), 0));
  cr_assert(eq(sz, o.n_hosts, 1));
  cr_assert(eq(str, o.hosts[0], "host"));
}

Test(value_options, size, .init = redirect_all) {
  char *argv[] = {"ft_ping", "-s", "100", "host", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(4, argv, &o), 0));
  cr_assert(eq(sz, o.n_hosts, 1));
  cr_assert(eq(str, o.hosts[0], "host"));
}

Test(value_options, interval, .init = redirect_all) {
  char *argv[] = {"ft_ping", "-i", "0.5", "host", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(4, argv, &o), 0));
  cr_assert(eq(sz, o.n_hosts, 1));
  cr_assert(eq(str, o.hosts[0], "host"));
}

Test(value_options, tos, .init = redirect_all) {
  char *argv[] = {"ft_ping", "-T", "16", "host", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(4, argv, &o), 0));
  cr_assert(eq(sz, o.n_hosts, 1));
  cr_assert(eq(str, o.hosts[0], "host"));
}

Test(value_options, timeout, .init = redirect_all) {
  char *argv[] = {"ft_ping", "-w", "10", "host", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(4, argv, &o), 0));
  cr_assert(eq(sz, o.n_hosts, 1));
  cr_assert(eq(str, o.hosts[0], "host"));
}

Test(value_options, linger, .init = redirect_all) {
  char *argv[] = {"ft_ping", "-W", "3", "host", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(4, argv, &o), 0));
  cr_assert(eq(sz, o.n_hosts, 1));
  cr_assert(eq(str, o.hosts[0], "host"));
}

Test(value_options, preload, .init = redirect_all) {
  char *argv[] = {"ft_ping", "-l", "5", "host", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(4, argv, &o), 0));
  cr_assert(eq(sz, o.n_hosts, 1));
  cr_assert(eq(str, o.hosts[0], "host"));
}

Test(value_options, pattern, .init = redirect_all) {
  char *argv[] = {"ft_ping", "-p", "ff", "host", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(4, argv, &o), 0));
  cr_assert(eq(sz, o.n_hosts, 1));
  cr_assert(eq(str, o.hosts[0], "host"));
}

Test(value_options, type, .init = redirect_all) {
  char *argv[] = {"ft_ping", "-t", "8", "host", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(4, argv, &o), 0));
  cr_assert(eq(sz, o.n_hosts, 1));
  cr_assert(eq(str, o.hosts[0], "host"));
}

Test(value_options, ip_timestamp, .init = redirect_all) {
  char *argv[] = {"ft_ping", "--ip-timestamp", "tsonly", "host", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(4, argv, &o), 0));
  cr_assert(eq(sz, o.n_hosts, 1));
  cr_assert(eq(str, o.hosts[0], "host"));
}

/* --router is a hidden, not-yet-implemented flag: it must be accepted rather
   than rejected as an unknown option. */
Test(value_options, router_hidden_placeholder, .init = redirect_all) {
  char *argv[] = {"ft_ping", "--router", "host", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(3, argv, &o), 0));
  cr_assert(eq(sz, o.n_hosts, 1));
  cr_assert(eq(str, o.hosts[0], "host"));
}

Test(value_options, count_glued_short, .init = redirect_all) {
  char *argv[] = {"ft_ping", "-c5", "host", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(3, argv, &o), 0));
  cr_assert(eq(sz, o.n_hosts, 1));
  cr_assert(eq(str, o.hosts[0], "host"));
}

Test(value_options, count_long_equals, .init = redirect_all) {
  char *argv[] = {"ft_ping", "--count=5", "host", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(3, argv, &o), 0));
  cr_assert(eq(sz, o.n_hosts, 1));
  cr_assert(eq(str, o.hosts[0], "host"));
}

/* -- separator -- */

Test(separator, dash_dash_turns_flag_into_operand) {
  char *argv[] = {"ft_ping", "--", "-v", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(3, argv, &o), 0));
  cr_assert(eq(sz, o.n_hosts, 1));
  cr_assert(eq(str, o.hosts[0], "-v"));
  cr_assert(eq(int, (int)o.flags, 0));
}

Test(separator, dash_dash_multiple_operands) {
  char *argv[] = {"ft_ping", "--", "-h", "--foo", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(4, argv, &o), 0));
  cr_assert(eq(sz, o.n_hosts, 2));
  cr_assert(eq(str, o.hosts[0], "-h"));
  cr_assert(eq(str, o.hosts[1], "--foo"));
  cr_assert(eq(int, (int)o.flags, 0));
}

/* -- reentrance -- */

Test(reentrance, second_parse_resets_flags_and_hosts, .init = redirect_all) {
  char *argv1[] = {"ft_ping", "-v", "host1", NULL};
  char *argv2[] = {"ft_ping", "host2", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(3, argv1, &o), 0));
  cr_assert(eq(int, (int)(o.flags & OPT_VERBOSE), OPT_VERBOSE));
  cr_assert(eq(sz, o.n_hosts, 1));
  cr_assert(eq(str, o.hosts[0], "host1"));

  /* Re-parsing the same record must wipe the previous state. */
  cr_assert(eq(int, options_parse(2, argv2, &o), 0));
  cr_assert(eq(int, (int)o.flags, 0));
  cr_assert(eq(sz, o.n_hosts, 1));
  cr_assert(eq(str, o.hosts[0], "host2"));
  cr_assert(eq(int, o.action, ACT_PING));
}

Test(reentrance, second_parse_resets_action, .init = redirect_all) {
  char *argv1[] = {"ft_ping", "--version", NULL};
  char *argv2[] = {"ft_ping", "host", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(2, argv1, &o), 0));
  cr_assert(eq(int, o.action, ACT_VERSION));

  cr_assert(eq(int, options_parse(2, argv2, &o), 0));
  cr_assert(eq(int, o.action, ACT_PING));
}
