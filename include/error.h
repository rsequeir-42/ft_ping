/*
  ft_ping - reporting of fatal command-line errors.

  argp prints its own native diagnostics (unknown option, ...). These
  helpers cover the messages ft_ping emits on its own, so that they share
  a single format with the rest of the program.
*/

#ifndef ERROR_H
#define ERROR_H

/*
  Print "PROG: MESSAGE" on stderr, followed by the standard
  "Try 'PROG --help' or 'PROG --usage' for more information." hint.
  PROG is the short program name (argv[0] basename).
*/
void error_report(const char *prog, const char *message);

/*
  Print "PROG: MESSAGE" on stderr, with NO "Try ..." hint. For value
  errors (a bad option argument), which exit with status 1. FORMAT work:
  like printf.
*/
void error_value(const char *prog, const char *format, ...) __attribute__((format(printf, 2, 3)));

/* Print only the "Try 'PROG --help' ..." hint line on stderr. */
void error_try_hint(const char *prog);

#endif /* ERROR_H */
