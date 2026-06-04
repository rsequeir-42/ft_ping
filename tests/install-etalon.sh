#!/usr/bin/env bash
# =============================================================================
# Install the inetutils-2.0 reference `ping` (the conformance etalon), isolated.
# =============================================================================
# Compiles GNU inetutils 2.0 from the signed tarball and installs ONLY its ping,
# under a dedicated name -- it never overwrites the system ping. The conformance
# tests will run ft_ping against this exact reference.
#
# Override with environment variables (sane defaults otherwise):
#   FT_PING_ETALON          install path (default ~/.local/bin/inetutils-ping)
#   FT_PING_ETALON_WORKDIR  build directory (default $TMPDIR/ft_ping-etalong)
#   FT_PING_ETALON_MIRROR   GNU mirror (default ftp.gnu.org)
#
# Idempotent : if the etalon is already at version 2.0 it only re-applies the
# capability (which a rebuild/copy drops) and stops. Needs the build deps from
# `make bootstrap` (build-essential, xz-utils, gnubg, libcap2-bin). Never uses
# apt for inetutils-ping -- that would remove iputils-ping + ubuntu-minimal.
# =============================================================================
set -euo pipefail

VERSION="2.0"
ETALON_BIN="${FT_PING_ETALON:-$HOME/.local/bin/inetutils-ping}"
WORKDIR="${FT_PING_ETALON_WORKDIR:-${TMPDIR:-/tmp}/ft_ping-etalon}"
MIRROR="${FT_PING_ETALON_MIRROR:-https://ftp.gnu.org/gnu/inetutils}"
KEYRING_URL="https://ftp.gnu.org/gnu/gnu-keyring.gpg"
TARBALL="inetutils-${VERSION}.tar.xz"
# Pinned sha256 -- local integrity guard; authenticity comes from the GPG check.
TARBALL_SHA256="e573d566e55393940099862e7f8994164a0ed12f5a86c3345380842bdc124722"

# --- 0. Already installed? Re-apply the capability and stop. -----------------
if [ -x "$ETALON_BIN" ] && "$ETALON_BIN" --version 2>/dev/null | grep -q "inetutils) $VERSION"; then
    echo ">>> inetutils-ping $VERSION already installed: $ETALON_BIN"
    sudo setcap cap_net_raw+ep "$ETALON_BIN"
    echo ">>> capability re-applied. Done."
    exit 0
fi

# --- 1. Required tools (provisioned by 'make bootstrap'). --------------------
for c in curl gpgv tar xz make cc setcap; do
    command -v "$c" >/dev/null 2>&1 || { echo "missing: $c -> run 'make bootstrap'" >&2; exit 1; }
done

# --- 2. Download tarball + signature + GNU keyring. --------------------------
mkdir -p "$WORKDIR"; cd "$WORKDIR"
[ -f "$TARBALL" ] || curl -fsSL -o "$TARBALL" "$MIRROR/$TARBALL"
curl -fsSL -o "$TARBALL.sig" "$MIRROR/$TARBALL.sig"
curl -fsSL -o gnu-keyring.gpg "$KEYRING_URL"

# --- 3. Authenticity (GPG, offline) then integrity (sha256). -----------------
# gpgv validates the signature against the GNU keyring and ignores the web of
# trust -> a clear verdict. The tarball is signed by Simon Josefsson (inetutils
# maintainer); only a BAD signature aborts.
echo ">>> verifying signature (gpgv + gnu-keyring)"
gpgv --keyring ./gnu-keyring.gpg "$TARBALL.sig" "$TARBALL"
echo ">>> verifying sha256"
echo "$TARBALL_SHA256  $TARBALL" | sha256sum -c -

# --- 4. Build ping. configure disables everything but ping, so a plain `make`
# builds only ping and the internal libs it needs (libicmp, lib) -- a `make -C
# ping` alone fails because it doesn't build those dependencies. Output goes to
# a log to keep things quiet; shown only on failure.
tar xJf "$TARBALL"
cd "inetutils-${VERSION}"
echo ">>> building inetutils ping (configure + make)..."
./configure -q --disable-servers --disable-clients --enable-ping >"$WORKDIR/build.log" 2>&1 \
    || { echo "configure failed:" >&2; tail -20 "$WORKDIR/build.log" >&2; exit 1; }
make >>"$WORKDIR/build.log" 2>&1 \
    || { echo "build failed:" >&2; tail -20 "$WORKDIR/build.log" >&2; exit 1; }

# --- 5. Install the binary (copy, never 'make install') + capability. --------
mkdir -p "$(dirname "$ETALON_BIN")"
cp ping/ping "$ETALON_BIN"
sudo setcap cap_net_raw+ep "$ETALON_BIN"

# --- 6. Verify. --------------------------------------------------------------
"$ETALON_BIN" --version | head -1
getcap "$ETALON_BIN"
echo ">>> etalon installed: $ETALON_BIN"