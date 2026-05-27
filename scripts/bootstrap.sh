#!/usr/bin/env bash
# Bootstraps the ft_ping dev environment: installs Ansible if missing, then runs
# the provisioning playbook on the CURRENT machine (local connection).
# Separate from the build: 'make all' never provisions the system.
set -euo pipefail

readonly PROVISIONING_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../provisioning" && pwd)"

# Use apt's Ansible (consistent with the Debian VM, which only ships apt's).
# Prefixing /usr/bin hides any other 'ansible' earlier in PATH (e.g. a global
# 'uv tool' install on the dev host): that CLI-vs-module version split is what
# breaks ansible-lint. Harmless on the VM (ansible is already in /usr/bin). The
# export dies with this subprocess -> the user's shell and uv tool stay intact.
export PATH="/usr/bin:$PATH"

# 1. Ansible present? Otherwise install it via system apt.
if ! command -v ansible-playbook >/dev/null 2>&1; then
    echo ">>> Ansible missing: installing via apt..."
    sudo apt-get update
    sudo apt-get install -y ansible
fi

# 2. Run the playbook locally.
#    'become' escalates via sudo. We add --ask-become-pass ONLY when sudo really
#    needs a password: if 'sudo -n true' succeeds (cached credentials or NOPASSWD),
#    no prompt -> no getpass call -> no 'GetPassWarning: can not control echo'.
#    When a password IS required, run this from a real terminal so getpass can
#    disable echo via /dev/tty (an IDE's integrated terminal often cannot).
echo ">>> Provisioning the current machine..."
cd "$PROVISIONING_DIR"

become_opts=()
if ! sudo -n true 2>/dev/null; then
    become_opts+=(--ask-become-pass)
fi
ansible-playbook playbook.yml "${become_opts[@]}" "$@"

echo ">>> Done. Check idempotence: run again and expect 'changed=0'."
