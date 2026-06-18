/*
    ft_ping - reporting of fatal command-line errors.

    These mirror the messages glibc's argp would print for our own errors,
    but with a fixed straight-quote rendering so the output is stable
    regardless of the active locale's quoting characters.
*/

#include "error.h"

#include <stdio.h>

void error_try_hint(const char *prog) {
  (void)fprintf(stderr, "Try '%s --help' or '%s --usage' for more information.\n", prog, prog);
}

void error_report(const char *prog, const char *message) {
  (void)fprintf(stderr, "%s: %s\n", prog, message);
  error_try_hint(prog);
}
