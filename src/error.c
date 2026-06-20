/*
  ft_ping - reporting of fatal command-line errors.

  These mirror the messages glibc's argp would print for our own errors,
  but with a fixed straight-quote rendering so the output is stable
  regardless of the active locale's quoting characters.
*/

#include "error.h"

#include <stdarg.h>
#include <stdio.h>

void error_try_hint(const char *prog) {
  (void)fprintf(stderr, "Try '%s --help' or '%s --usage' for more information.\n", prog, prog);
}

void error_value(const char *prog, const char *format, ...) {
  va_list ap;

  (void)fprintf(stderr, "%s: ", prog);
  va_start(ap, format);
  (void)vfprintf(stderr, format, ap);
  va_end(ap);
  (void)fputc('\n', stderr);
}

void error_report(const char *prog, const char *message) {
  error_value(prog, "%s", message);
  error_try_hint(prog);
}
