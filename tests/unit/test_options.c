/*
  Unit tests for the command-line parser, options_parse().

  They exercise the flags, the request types, the help/usage/version
  actions, operand collection, the "--" separator, reentrance, and the
  reading/validation/storage of the options that take an argument: stored
  values, bounds, rejected inputs and the 1-vs-64 exit codes.

  Conventions:
      - argv is a local array of string literals terminated by NULL; argc is
        the count without the trailing NULL. argp only reorders the pointers,
        never the strings, so literals are safe here.
      - error paths print on stderr; the redirect_all init swallows that so
        the diagnostics never reach the test output. Exact message text is
        not asserted here (left to a black-box bats suite); these tests check
        the exit code and the effect on the record.
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
  cr_assert(eq(sz, o.interval, 1000));
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

/* -- value_stored: a valid argument is read, validated and stored -- */

Test(value_stored, count) {
  char *argv[] = {"ft_ping", "-c", "5", "host", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(4, argv, &o), 0));
  cr_assert(eq(sz, o.count, 5));
}

Test(value_stored, size) {
  char *argv[] = {"ft_ping", "-s", "100", "host", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(4, argv, &o), 0));
  cr_assert(eq(sz, o.data_length, 100));
}

Test(value_stored, interval_seconds_to_ms) {
  char *argv[] = {"ft_ping", "-i", "2", "host", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(4, argv, &o), 0));
  cr_assert(eq(sz, o.interval, 2000));
  cr_assert(eq(int, (int)(o.flags & OPT_INTERVAL), OPT_INTERVAL));
}

Test(value_stored, interval_fractional) {
  char *argv[] = {"ft_ping", "-i", "0.5", "host", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(4, argv, &o), 0));
  cr_assert(eq(sz, o.interval, 500));
}

Test(value_stored, tos) {
  char *argv[] = {"ft_ping", "-T", "16", "host", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(4, argv, &o), 0));
  cr_assert(eq(int, o.tos, 16));
}

Test(value_stored, ttl) {
  char *argv[] = {"ft_ping", "--ttl", "64", "host", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(4, argv, &o), 0));
  cr_assert(eq(int, o.ttl, 64));
}

Test(value_stored, timeout) {
  char *argv[] = {"ft_ping", "-w", "10", "host", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(4, argv, &o), 0));
  cr_assert(eq(int, o.timeout, 10));
}

Test(value_stored, linger) {
  char *argv[] = {"ft_ping", "-W", "3", "host", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(4, argv, &o), 0));
  cr_assert(eq(int, o.linger, 3));
}

Test(value_stored, preload) {
  char *argv[] = {"ft_ping", "-l", "5", "host", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(4, argv, &o), 0));
  cr_assert(eq(int, (int)o.preload, 5));
}

Test(value_stored, pattern_two_bytes) {
  char *argv[] = {"ft_ping", "-p", "a1b2", "host", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(4, argv, &o), 0));
  cr_assert(eq(int, o.pattern_len, 2));
  cr_assert(eq(int, o.pattern[0], 0xa1));
  cr_assert(eq(int, o.pattern[1], 0xb2));
}

Test(value_stored, pattern_single_digit) {
  char *argv[] = {"ft_ping", "-p", "f", "host", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(4, argv, &o), 0));
  cr_assert(eq(int, o.pattern_len, 1));
  cr_assert(eq(int, o.pattern[0], 0x0f));
}

Test(value_stored, type_echo) {
  char *argv[] = {"ft_ping", "-t", "echo", "host", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(4, argv, &o), 0));
  cr_assert(eq(int, o.type, PING_ECHO));
}

Test(value_stored, type_timestamp) {
  char *argv[] = {"ft_ping", "-t", "timestamp", "host", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(4, argv, &o), 0));
  cr_assert(eq(int, o.type, PING_TIMESTAMP));
}

Test(value_stored, type_address) {
  char *argv[] = {"ft_ping", "-t", "address", "host", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(4, argv, &o), 0));
  cr_assert(eq(int, o.type, PING_ADDRESS));
}

Test(value_stored, type_case_insensitive) {
  char *argv[] = {"ft_ping", "-t", "ECHO", "host", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(4, argv, &o), 0));
  cr_assert(eq(int, o.type, PING_ECHO));
}

Test(value_stored, ip_timestamp_tsonly) {
  char *argv[] = {"ft_ping", "--ip-timestamp", "tsonly", "host", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(4, argv, &o), 0));
  cr_assert(eq(int, (int)(o.flags & OPT_IPTIMESTAMP), OPT_IPTIMESTAMP));
}

Test(value_stored, ip_timestamp_tsaddr) {
  char *argv[] = {"ft_ping", "--ip-timestamp", "tsaddr", "host", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(4, argv, &o), 0));
  cr_assert(eq(int, (int)(o.flags & OPT_IPTIMESTAMP), OPT_IPTIMESTAMP));
}

/* -- value_forms: argument spellings (glued, =, hex, octal via base 0) -- */

Test(value_forms, glued_short) {
  char *argv[] = {"ft_ping", "-c5", "host", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(3, argv, &o), 0));
  cr_assert(eq(sz, o.count, 5));
}

Test(value_forms, long_equals) {
  char *argv[] = {"ft_ping", "--count=5", "host", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(3, argv, &o), 0));
  cr_assert(eq(sz, o.count, 5));
}

Test(value_forms, hex_base0) {
  char *argv[] = {"ft_ping", "-c", "0x10", "host", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(4, argv, &o), 0));
  cr_assert(eq(sz, o.count, 16));
}

Test(value_forms, octal_base0) {
  char *argv[] = {"ft_ping", "-c", "010", "host", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(4, argv, &o), 0));
  cr_assert(eq(sz, o.count, 8));
}

/* -- value_bounds: just-below / at / above each limit -- */

Test(value_bounds, size_at_max_ok) {
  char *argv[] = {"ft_ping", "-s", "65399", "host", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(4, argv, &o), 0));
  cr_assert(eq(sz, o.data_length, 65399));
}

Test(value_bounds, size_above_max_rejected, .init = redirect_all) {
  char *argv[] = {"ft_ping", "-s", "65400", "host", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(4, argv, &o), 1));
}

Test(value_bounds, tos_at_max_ok) {
  char *argv[] = {"ft_ping", "-T", "255", "host", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(4, argv, &o), 0));
  cr_assert(eq(int, o.tos, 255));
}

Test(value_bounds, tos_above_max_rejected, .init = redirect_all) {
  char *argv[] = {"ft_ping", "-T", "256", "host", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(4, argv, &o), 1));
}

Test(value_bounds, ttl_min_ok) {
  char *argv[] = {"ft_ping", "--ttl", "1", "host", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(4, argv, &o), 0));
  cr_assert(eq(int, o.ttl, 1));
}

Test(value_bounds, ttl_zero_rejected, .init = redirect_all) {
  char *argv[] = {"ft_ping", "--ttl", "0", "host", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(4, argv, &o), 1));
}

Test(value_bounds, ttl_above_max_rejected, .init = redirect_all) {
  char *argv[] = {"ft_ping", "--ttl", "256", "host", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(4, argv, &o), 1));
}

Test(value_bounds, count_zero_ok) {
  char *argv[] = {"ft_ping", "-c", "0", "host", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(4, argv, &o), 0));
  cr_assert(eq(sz, o.count, 0));
}

Test(value_bounds, timeout_zero_rejected, .init = redirect_all) {
  char *argv[] = {"ft_ping", "-w", "0", "host", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(4, argv, &o), 1));
}

Test(value_bounds, linger_zero_rejected, .init = redirect_all) {
  char *argv[] = {"ft_ping", "-W", "0", "host", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(4, argv, &o), 1));
}

/* -- value_errors: malformed values are rejected with status 1 -- */

Test(value_errors, count_not_a_number, .init = redirect_all) {
  char *argv[] = {"ft_ping", "-c", "abc", "host", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(4, argv, &o), 1));
}

Test(value_errors, count_trailing_junk, .init = redirect_all) {
  char *argv[] = {"ft_ping", "-c", "5x", "host", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(4, argv, &o), 1));
}

Test(value_errors, count_negative, .init = redirect_all) {
  char *argv[] = {"ft_ping", "--count=-1", "host", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(3, argv, &o), 1));
}

Test(value_errors, count_overflow, .init = redirect_all) {
  char *argv[] = {"ft_ping", "-c", "99999999999999999999", "host", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(4, argv, &o), 1));
}

Test(value_errors, size_negative, .init = redirect_all) {
  char *argv[] = {"ft_ping", "--size=-1", "host", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(3, argv, &o), 1));
}

Test(value_errors, preload_not_a_number, .init = redirect_all) {
  char *argv[] = {"ft_ping", "-l", "abc", "host", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(4, argv, &o), 1));
}

Test(value_errors, preload_negative, .init = redirect_all) {
  char *argv[] = {"ft_ping", "--preload=-1", "host", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(3, argv, &o), 1));
}

Test(value_errors, interval_not_a_number, .init = redirect_all) {
  char *argv[] = {"ft_ping", "-i", "abc", "host", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(4, argv, &o), 1));
}

Test(value_errors, interval_negative, .init = redirect_all) {
  char *argv[] = {"ft_ping", "--interval=-1", "host", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(3, argv, &o), 1));
}

Test(value_errors, pattern_non_hex, .init = redirect_all) {
  char *argv[] = {"ft_ping", "-p", "xyz", "host", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(4, argv, &o), 1));
}

Test(value_errors, pattern_too_long, .init = redirect_all) {
  char *argv[] = {"ft_ping", "-p", "000102030405060708090a0b0c0d0e0f10", "host", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(4, argv, &o), 1));
}

Test(value_errors, type_unsupported, .init = redirect_all) {
  char *argv[] = {"ft_ping", "-t", "bogus", "host", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(4, argv, &o), 1));
}

Test(value_errors, ip_timestamp_unsupported, .init = redirect_all) {
  char *argv[] = {"ft_ping", "--ip-timestamp", "bogus", "host", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(4, argv, &o), 1));
}

/* --router maps to an unsupported request type (inetutils rejects it too). */
Test(value_errors, router_unsupported, .init = redirect_all) {
  char *argv[] = {"ft_ping", "--router", "host", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(3, argv, &o), 1));
}

/* -- value_codes: a bad value exits 1, a usage error exits 64 -- */

Test(value_codes, bad_value_is_1, .init = redirect_all) {
  char *argv[] = {"ft_ping", "-c", "abc", "host", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(4, argv, &o), 1));
}

Test(value_codes, unknown_option_is_64, .init = redirect_all) {
  char *argv[] = {"ft_ping", "-Z", "host", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(3, argv, &o), 64));
}

Test(value_codes, missing_argument_is_64, .init = redirect_all) {
  char *argv[] = {"ft_ping", "--count", NULL};
  t_options o;

  cr_assert(eq(int, options_parse(2, argv, &o), 64));
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
