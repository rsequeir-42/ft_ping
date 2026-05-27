# preseed.cfg.tpl - unattended Debian 13 (trixie) install for the ft_ping VM.
# scripts/create-vm.sh substitutes these placeholders before the install:
#   USERNAME / FULLNAME  - the sudo user (no root login) and its full name
#   PW_HASH              - SHA-512 password hash (openssl passwd -6)
#   SSH_PUBKEY           - public SSH key (lets Ansible reach the VM over SSH)

# --- Localization: en_US.UTF-8. The serial console used to log the install only
# offers ASCII languages, so the VM installs in English; fr_FR.UTF-8 (for
# ft_ping's comma-decimal output) is generated later by the provisioning. ---
d-i debian-installer/locale string en_US.UTF-8
d-i keyboard-configuration/xkb-keymap select us

# --- Network: DHCP, fixed hostname ---
d-i netcfg/choose_interface select auto
d-i netcfg/get_hostname string ft-ping
d-i netcfg/get_domain string localdomain

# --- Mirror ---
d-i mirror/country string manual
d-i mirror/http/hostname string deb.debian.org
d-i mirror/http/directory string /debian
d-i mirror/http/proxy string

# --- Clock ---
d-i clock-setup/utc boolean true
d-i time/zone string Europe/Paris
d-i clock-setup/ntp boolean true

# --- Accounts: root login disabled, one sudo user ---
d-i passwd/root-login boolean false
d-i passwd/make-user boolean true
d-i passwd/user-fullname string @@FULLNAME@@
d-i passwd/username string @@USERNAME@@
d-i passwd/user-password-crypted password @@PW_HASH@@

# --- Partitioning: guided, all files in one partition, whole disk ---
d-i partman-auto/method string regular
d-i partman-auto/choose_recipe select atomic
d-i partman-partitioning/confirm_write_new_label boolean true
d-i partman/choose_partition select finish
d-i partman/confirm boolean true
d-i partman/confirm_nooverwrite boolean true

# --- Base packages: SSH (Ansible transport), sudo, python3 (Ansible modules) ---
tasksel tasksel/first multiselect standard
d-i pkgsel/include string openssh-server sudo python3 ca-certificates
popularity-contest popularity-contest/participate boolean false

# --- Bootloader ---
d-i grub-installer/only_debian boolean true
d-i grub-installer/bootdev string default

# --- Finish without prompting, then reboot ---
d-i finish-install/reboot_in_progress note

# --- late_command: install the SSH key and grant passwordless sudo to the user ---
d-i preseed/late_command string \
    in-target sh -c 'install -d -m 700 -o @@USERNAME@@ -g @@USERNAME@@ /home/@@USERNAME@@/.ssh'; \
    in-target sh -c 'echo "@@SSH_PUBKEY@@" > /home/@@USERNAME@@/.ssh/authorized_keys'; \
    in-target sh -c 'chown @@USERNAME@@:@@USERNAME@@ /home/@@USERNAME@@/.ssh/authorized_keys; chmod 600 /home/@@USERNAME@@/.ssh/authorized_keys'; \
    in-target sh -c 'echo "@@USERNAME@@ ALL=(ALL) NOPASSWD:ALL" > /etc/sudoers.d/@@USERNAME@@; chmod 440 /etc/sudoers.d/@@USERNAME@@'
