#!/usr/bin/env bash
# =============================================================================
# Create, install and provision a reproducible Debian 13 (trixie) VM.
# =============================================================================
# One command, same flow on the personal machine and on a 42 workstation:
#   1. fetch the official netinst ISO (+ checksum, cached)
#   2. generate an SSH key and the preseed from provisioning/preseed.cfg.tpl
#   3. remaster the ISO: preseed inside the initrd + non-interactive boot
#   4. create the VM (VBoxManage), boot it headless, install unattended
#   5. wait over SSH, detach the ISO, provision the toolchain with Ansible
#
# Everything runs as a regular user (VirtualBox preinstalled, user in
# 'vboxusers'). The ISO and the disk live outside the repo (FTPING_VM_DIR,
# defaults to ~/.cache/ftping-vm; on 42 set it to ~/goinfre/...). They are
# disposable: rerunning this script rebuilds the VM from scratch.
#
# Usage:
#   scripts/create-vm.sh                 # full run (create + install + provision)
#   scripts/create-vm.sh --prepare-only  # stop after the remaster (no VM)
#
# Tunables (environment variables, with defaults):
#   FTPING_VM_DIR FTPING_VM_NAME FTPING_VM_RAM FTPING_VM_CPUS FTPING_VM_DISK
#   FTPING_VM_SSH_PORT FTPING_VM_USER FTPING_VM_PASSWORD FTPING_VM_FULLNAME
# =============================================================================
set -euo pipefail

# --- Configuration -----------------------------------------------------------
WORKDIR="${FTPING_VM_DIR:-$HOME/.cache/ftping-vm}"
VM_NAME="${FTPING_VM_NAME:-ft_ping-trixie}"
RAM_MB="${FTPING_VM_RAM:-2048}"
CPUS="${FTPING_VM_CPUS:-2}"
DISK_MB="${FTPING_VM_DISK:-15000}"
SSH_PORT="${FTPING_VM_SSH_PORT:-2222}"
VM_USER="${FTPING_VM_USER:-ftping}"
# Console password: random per run so no secret lands in the repo (SSH uses the
# key anyway). Override with FTPING_VM_PASSWORD; printed at the end for console use.
VM_PASSWORD="${FTPING_VM_PASSWORD:-$(openssl rand -hex 8)}"
VM_FULLNAME="${FTPING_VM_FULLNAME:-ft_ping developer}"
# Debian image mirror: canonical default, override with FTPING_VM_MIRROR for a
# local mirror/cache. A proxy needs no change here -- curl honors http(s)_proxy.
MIRROR="${FTPING_VM_MIRROR:-https://cdimage.debian.org/debian-cd/current/amd64/iso-cd}"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROVISIONING_DIR="$(cd "$SCRIPT_DIR/../provisioning" && pwd)"
SERIAL_LOG="$WORKDIR/$VM_NAME-serial.log"

PREPARE_ONLY=0
[ "${1:-}" = "--prepare-only" ] && PREPARE_ONLY=1

# --- Steps -------------------------------------------------------------------

# Fail early with a clear list of missing tools (mode-dependent: --prepare-only
# only needs the remaster tools, not VirtualBox/Ansible).
require_tools() {
    local tools="xorriso curl openssl cpio gzip ssh-keygen sha256sum sed"
    [ "$PREPARE_ONLY" -eq 1 ] || tools="$tools VBoxManage ssh ansible-playbook"
    local missing=0 t
    for t in $tools; do
        command -v "$t" >/dev/null 2>&1 || { echo "  missing: $t" >&2; missing=1; }
    done
    [ "$missing" -eq 0 ] || { echo "Install the tools above and retry." >&2; exit 1; }
}

# Resolve the current netinst ISO name, download it if absent, verify checksum.
download_iso() {
    mkdir -p "$WORKDIR"
    ISO_NAME="$(curl -fsSL "$MIRROR/" \
        | grep -oE 'debian-[0-9.]+-amd64-netinst\.iso' | head -1)"
    ISO_PATH="$WORKDIR/$ISO_NAME"
    if [ ! -f "$ISO_PATH" ]; then
        echo ">>> downloading $ISO_NAME"
        curl -fL -o "$ISO_PATH" "$MIRROR/$ISO_NAME"
    fi
    echo ">>> verifying checksum"
    curl -fsSL "$MIRROR/SHA256SUMS" -o "$WORKDIR/SHA256SUMS"
    ( cd "$WORKDIR" && grep " $ISO_NAME\$" SHA256SUMS | sha256sum -c - )
}

# Generate an ed25519 key once; its public half goes into the preseed, its
# private half lets Ansible reach the VM.
ensure_ssh_key() {
    SSH_KEY="$WORKDIR/id_ed25519"
    [ -f "$SSH_KEY" ] || ssh-keygen -t ed25519 -N "" -C "ftping-vm" -f "$SSH_KEY"
    SSH_PUBKEY="$(cat "$SSH_KEY.pub")"
}

# Fill the preseed template. '|' delimiter: the values (user, hash, base64 key)
# never contain it. The SHA-512 hash is what the installer stores for the user.
build_preseed() {
    PRESEED="$WORKDIR/preseed.cfg"
    local pw_hash
    pw_hash="$(openssl passwd -6 "$VM_PASSWORD")"
    sed -e "s|@@USERNAME@@|$VM_USER|g" \
        -e "s|@@FULLNAME@@|$VM_FULLNAME|g" \
        -e "s|@@PW_HASH@@|$pw_hash|g" \
        -e "s|@@SSH_PUBKEY@@|$SSH_PUBKEY|g" \
        "$PROVISIONING_DIR/preseed.cfg.tpl" > "$PRESEED"
}

# Remaster the ISO: inject the preseed into the initrd and force a direct,
# non-interactive boot of the text installer. 'replay' keeps the original
# El Torito / isohybrid boot record, so the result stays BIOS+UEFI bootable.
remaster_iso() {
    PRESEED_ISO="$WORKDIR/preseed-$ISO_NAME"
    local edit="$WORKDIR/remaster"
    # Start clean: xorriso -outdev on an existing file treats it as an
    # appendable medium and fails, so drop a previous output ISO first.
    rm -rf "$edit"; rm -f "$PRESEED_ISO"; mkdir -p "$edit/stage"
    cp "$PRESEED" "$edit/stage/preseed.cfg"

    echo ">>> remastering $ISO_NAME"
    xorriso -osirrox on -indev "$ISO_PATH" \
        -extract /install.amd/initrd.gz "$edit/initrd.gz" \
        -extract /isolinux/isolinux.cfg "$edit/isolinux.cfg" \
        -extract /isolinux/txt.cfg "$edit/txt.cfg" \
        -extract /boot/grub/grub.cfg "$edit/grub.cfg" 2>/dev/null

    # d-i auto-loads /preseed.cfg from the initrd root. xorriso extracts the
    # initrd read-only (it preserves the ISO file mode), so make it writable
    # before cpio -A can append into it. --quiet drops the "N blocks" noise
    # without hiding real errors.
    gunzip -f "$edit/initrd.gz"
    chmod u+w "$edit/initrd"
    ( cd "$edit/stage" && echo preseed.cfg \
        | cpio -H newc -o -A -F "$edit/initrd" --quiet )
    gzip -f "$edit/initrd"

    # isolinux (BIOS): boot the 'install' label directly (workaround for bug
    # #18410). The append carries: auto=true + priority=critical; the text
    # frontend (readable serial log); the localization in en_US -- the serial
    # console only offers ASCII languages (C/English), so a French install is
    # impossible here (localechooser rejects 'fr' as an unknown language code);
    # fr_FR.UTF-8 is generated at provisioning time instead. Plus a serial
    # console (ttyS0) logging the install to the host's COM1 file.
    sed -i 's/^default vesamenu.c32/default install/; s/^timeout 0/timeout 1/' \
        "$edit/isolinux.cfg"
    sed -i 's#append vga=788#append auto=true priority=critical DEBIAN_FRONTEND=text debian-installer/language=en debian-installer/country=US debian-installer/locale=en_US.UTF-8 keyboard-configuration/xkb-keymap=us console=tty0 console=ttyS0,115200 vga=788#' \
        "$edit/txt.cfg"

    # grub (UEFI): prepend an automated entry (same args as isolinux) as default.
    { printf 'set default=0\nset timeout=1\nmenuentry "ft_ping automated install" {\n    linux /install.amd/vmlinuz auto=true priority=critical DEBIAN_FRONTEND=text debian-installer/language=en debian-installer/country=US debian-installer/locale=en_US.UTF-8 keyboard-configuration/xkb-keymap=us console=tty0 console=ttyS0,115200 vga=788 --- quiet\n    initrd /install.amd/initrd.gz\n}\n'; cat "$edit/grub.cfg"; } > "$edit/grub.cfg.new"
    mv "$edit/grub.cfg.new" "$edit/grub.cfg"

    xorriso -indev "$ISO_PATH" -outdev "$PRESEED_ISO" -boot_image any replay \
        -update "$edit/initrd.gz" /install.amd/initrd.gz \
        -update "$edit/isolinux.cfg" /isolinux/isolinux.cfg \
        -update "$edit/txt.cfg" /isolinux/txt.cfg \
        -update "$edit/grub.cfg" /boot/grub/grub.cfg \
        -commit 2>/dev/null
}

# Recreate the VM from scratch (idempotent): drop any previous VM/disk first.
create_vm() {
    local disk="$WORKDIR/$VM_NAME.vdi"
    if VBoxManage showvminfo "$VM_NAME" >/dev/null 2>&1; then
        # A leftover VM may still be running (e.g. a stuck install): power it
        # off and wait for the lock to clear before removing it.
        VBoxManage controlvm "$VM_NAME" poweroff 2>/dev/null || true
        while VBoxManage list runningvms | grep -q "\"$VM_NAME\""; do sleep 1; done
        VBoxManage unregistervm "$VM_NAME" --delete
    fi
    VBoxManage closemedium disk "$disk" --delete 2>/dev/null || true
    rm -f "$disk"

    echo ">>> creating VM '$VM_NAME'"
    VBoxManage createvm --name "$VM_NAME" --ostype Debian_64 --register
    # Boot disk first: empty on the first boot -> falls through to the DVD
    # (install); after install the disk boots Debian -> no reinstall loop.
    VBoxManage modifyvm "$VM_NAME" --memory "$RAM_MB" --cpus "$CPUS" \
        --nic1 nat --boot1 disk --boot2 dvd --boot3 none --boot4 none
    VBoxManage modifyvm "$VM_NAME" --natpf1 "ssh,tcp,,$SSH_PORT,,22"
    # COM1 -> file: capture the installer's console for a live, persistent log.
    VBoxManage modifyvm "$VM_NAME" --uart1 0x3F8 4 --uartmode1 file "$SERIAL_LOG"
    VBoxManage createmedium disk --filename "$disk" --size "$DISK_MB" --format VDI
    VBoxManage storagectl "$VM_NAME" --name SATA --add sata \
        --controller IntelAhci --portcount 2
    VBoxManage storageattach "$VM_NAME" --storagectl SATA --port 0 --device 0 \
        --type hdd --medium "$disk"
    # Hot-pluggable so the ISO can be detached later while the VM is running.
    VBoxManage storageattach "$VM_NAME" --storagectl SATA --port 1 --device 0 \
        --type dvddrive --medium "$PRESEED_ISO" --hotpluggable on
}

# True when the installed system answers SSH with our key.
ssh_ready() {
    ssh -o BatchMode=yes -o StrictHostKeyChecking=no \
        -o UserKnownHostsFile=/dev/null -o ConnectTimeout=5 \
        -i "$SSH_KEY" -p "$SSH_PORT" "$VM_USER@127.0.0.1" true 2>/dev/null
}

# Boot headless and wait for the unattended install + reboot to finish, tailing
# the VM serial console so the install progress shows live.
start_and_wait() {
    : > "$SERIAL_LOG"
    echo ">>> starting VM (unattended install, ~10-15 min); live serial log:"
    VBoxManage startvm "$VM_NAME" --type headless
    tail -n +1 -f "$SERIAL_LOG" &
    local tail_pid=$!
    local deadline=$((SECONDS + 1800))
    until ssh_ready; do
        if [ "$SECONDS" -ge "$deadline" ]; then
            kill "$tail_pid" 2>/dev/null || true
            echo "timeout waiting for SSH" >&2; exit 1
        fi
        sleep 15
    done
    kill "$tail_pid" 2>/dev/null || true
    echo ">>> VM reachable over SSH"
}

# Drop the install medium. Non-fatal: the disk-first boot order already prevents
# any reinstall loop, so a failed detach must not abort the whole run.
detach_iso() {
    VBoxManage storageattach "$VM_NAME" --storagectl SATA --port 1 --device 0 \
        --type dvddrive --medium none \
        || echo ">>> note: ISO not detached (harmless; boot order is disk-first)"
}

# Provision the toolchain over SSH. group_vars/all.yml (next to the playbook)
# supplies ftping_packages; '-i' overrides ansible.cfg's local inventory.
# PATH=/usr/bin uses apt's Ansible (matches the VM, dodges the CLI/module split).
provision() {
    local inv="$WORKDIR/vm-inventory.ini"
    cat > "$inv" <<EOF
[vm]
127.0.0.1 ansible_port=$SSH_PORT ansible_user=$VM_USER ansible_ssh_private_key_file=$SSH_KEY
[vm:vars]
ansible_connection=ssh
ansible_python_interpreter=/usr/bin/python3
ansible_ssh_common_args=-o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null
EOF
    echo ">>> provisioning the toolchain with Ansible"
    ( cd "$PROVISIONING_DIR" && PATH="/usr/bin:$PATH" \
        ansible-playbook -i "$inv" playbook.yml )
}

# --- Orchestration -----------------------------------------------------------
require_tools
download_iso
ensure_ssh_key
build_preseed
remaster_iso

if [ "$PREPARE_ONLY" -eq 1 ]; then
    echo ">>> prepare-only done:"
    echo "    preseed ISO : $PRESEED_ISO"
    echo "    preseed     : $PRESEED"
    echo "    ssh key     : $SSH_KEY"
    exit 0
fi

create_vm
start_and_wait
detach_iso
provision
echo ">>> VM '$VM_NAME' ready. Connect with:"
echo "    ssh -i $SSH_KEY -p $SSH_PORT $VM_USER@127.0.0.1"
echo "    console login: $VM_USER / $VM_PASSWORD"
