#!/usr/bin/env bash
#
# (Re)generate the help-screen snapshots from ft_ping, then cross-check the
# whole CLI surface against the inetutils-2.0 etalon -- the safety net behind
# the frozen snapshots. The check is INFORMATIONAL (it never fails): it prints
# every divergence so a human confirms only the expected ones remain
# (program name aside: bug-address, --version identity, argp's backtick, -w -5).
#
# Usage: tests/conformance/generate-snapshots.sh
#   FT_PING         override the binary  (default: ../../ft_ping)
#   FT_PING_ETALON  override the etalon  (default: ~/.local/bin/inetutils-ping)

set -euo pipefail

HERE="$(cd "$(dirname "$0")" && pwd)"
FT_PING="${FT_PING:-$HERE/../../ft_ping}"
ETALON="${FT_PING_ETALON:-$HOME/.local/bin/inetutils-ping}"
EXPECTED="$HERE/expected"
mkdir -p "$EXPECTED"

# Run a binary with argv[0] forced to "ft_ping", under LC_ALL=C.
as_ft_ping() { LC_ALL=C bash -c 'exec -a ft_ping "$0" "$@"' "$1" "${@:2}"; }

# --- 1. Regenerate the snapshots from ft_ping. -------------------------------
as_ft_ping "$FT_PING" --usage   > "$EXPECTED/usage.txt"
as_ft_ping "$FT_PING" --help    > "$EXPECTED/help.txt"
as_ft_ping "$FT_PING" --version > "$EXPECTED/version.txt"
echo ">>> snapshots regenerated under $EXPECTED"

# --- 2. Cross-check against the etalon (name normalized to ft_ping). ---------
if [ ! -x "$ETALON" ]; then
  echo ">>> etalon absent ($ETALON) -- skipping cross-check"
  exit 0
fi
ename="$(basename "$ETALON")"
echo ">>> cross-check vs etalon (any divergence below is for review):"
check() {
  local desc="$1"; shift
  local e f
  e="$(LC_ALL=C "$ETALON" "$@" 2>&1 | sed "s|$ETALON|ft_ping|g; s|$ename|ft_ping|g")" || true
  f="$(as_ft_ping "$FT_PING" "$@" 2>&1)" || true
  if [ "$e" = "$f" ]; then
    echo "  = $desc"
  else
    echo "  ~ $desc :"
    diff <(printf '%s' "$e") <(printf '%s' "$f") || true
  fi
}
check "--usage" --usage
check "--help" --help
check "--version" --version
check "missing host"
check "-c abc" -c abc
check "-s 99999" -s 99999
check "-Z" -Z
check "--nope" --nope
check "-w -5" -w -5
echo ">>> expected divergences: bug-address (--help), --version identity, argp backtick (-Z/--nope), -w -5 wording."
