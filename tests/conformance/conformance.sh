#!/usr/bin/env bash
#
# Black-box conformance tests for ft_ping's command-line surface.
#
# Complements the Criterion unit tests (white box: the parsing logic, in memory)
# by running the REAL binary and pinning its textual output byte-for-byte against
# snapshots established via the inetutils-2.0 etalon (see generate-snapshots.sh).
#
# A self-contained shell harness -- no test framework to install or vendor, so it
# runs anywhere bash does (the 42 repo takes no submodules). LC_ALL=C keeps glibc
# from translating argp's own messages.
#
#   FT_PING   override the binary (default: ../../ft_ping)
# Exit status: 0 if every case passes, 1 otherwise.

set -u

HERE="$(cd "$(dirname "$0")" && pwd)"
FT_PING="${FT_PING:-$HERE/../../ft_ping}"
EXPECTED="$HERE/expected"
export LC_ALL=C

pass=0
fail=0

# Run ft_ping with argv[0] forced to the bare name -- argp prints argv[0] VERBATIM
# for its own errors, so this keeps the snapshots free of any invocation path.
# stdout and stderr are captured together (like a single observed stream).
run_ft() {
  out="$(exec -a ft_ping "$FT_PING" "$@" 2>&1)"
  code=$?
}

# check <desc> <expected-code> <expected-output> [-- <ft_ping args...>]
check() {
  local desc="$1" ecode="$2" eout="$3"
  shift 3
  [ "${1:-}" = "--" ] && shift
  run_ft "$@"
  if [ "$code" = "$ecode" ] && [ "$out" = "$eout" ]; then
    printf 'ok   - %s\n' "$desc"
    pass=$((pass + 1))
  else
    printf 'FAIL - %s\n' "$desc"
    [ "$code" != "$ecode" ] && printf '       code: expected %s, got %s\n' "$ecode" "$code"
    if [ "$out" != "$eout" ]; then
      printf '       output diff (< expected / > got):\n'
      diff <(printf '%s' "$eout") <(printf '%s' "$out") | sed 's/^/       /'
    fi
    fail=$((fail + 1))
  fi
}

# check_screen <desc> <expected-file> -- <ft_ping args...>   (success, file snapshot)
check_screen() {
  local desc="$1" file="$2"
  shift 2
  [ "${1:-}" = "--" ] && shift
  check "$desc" 0 "$(<"$EXPECTED/$file")" -- "$@"
}

# --- Help screens (long output: file snapshots) ------------------------------
check_screen "--usage"             usage.txt   -- --usage
check_screen "--help"              help.txt    -- --help
check_screen "-? equals --help"    help.txt    -- -?
check_screen "--version"           version.txt -- --version
check_screen "-V equals --version" version.txt -- -V

# --- Error messages (short output: inline) -----------------------------------
check "missing host operand -> 64, our voice (straight quotes)" 64 \
"ft_ping: missing host operand
Try 'ft_ping --help' or 'ft_ping --usage' for more information."

check "non-numeric value (-c abc) -> 1" 1 \
"ft_ping: invalid value (\`abc' near \`abc')" -- -c abc

check "value above the cap (-s 99999) -> 1" 1 \
"ft_ping: option value too big: 99999" -- -s 99999

check "zero accepted for -c, then missing host stops it" 64 \
"ft_ping: missing host operand
Try 'ft_ping --help' or 'ft_ping --usage' for more information." -- -c 0

check "unknown short option (-Z) -> 64, argp's voice (backtick)" 64 \
"ft_ping: invalid option -- 'Z'
Try \`ft_ping --help' or \`ft_ping --usage' for more information." -- -Z

check "unknown long option (--nope) -> 64, argp's voice (backtick)" 64 \
"ft_ping: unrecognized option '--nope'
Try \`ft_ping --help' or \`ft_ping --usage' for more information." -- --nope

check "negative value rejected our way (-w -5) -> 1, not 'too big'" 1 \
"ft_ping: invalid value (\`-5' near \`-5')" -- -w -5

# --- Report ------------------------------------------------------------------
printf -- '---\n%d passed, %d failed\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
