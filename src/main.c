/*
    ft_ping - program entry point.

    A thin humble object: set the locale, parse the command line, then act
    on the outcome. The help, usage and version requests print to stdout and
    exit successfully; a parsing error exits with its status code; a normal
    ping run is wired to the network engine later.
*/

#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ft_ping.h"
#include "options.h"

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

  /* ACT_PING with a valid command line. The network engine lands later. */
  return 0;
}
