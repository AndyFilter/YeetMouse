#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

if [[ "${EUID:-$(id -u)}" -eq 0 ]]; then
    if [[ -n "${SUDO_USER:-}" && "${SUDO_USER}" != "root" ]]; then
        TARGET_USER="$SUDO_USER"
    else
        echo "Please run this script as a normal user, not directly as root."
        exit 1
    fi
else
    TARGET_USER="${USER:-$(id -un)}"
fi

# Read version from a single source of truth
DKMS_VER="$(sed -n 's/^YEETMOUSE_VERSION[[:space:]]*:=[[:space:]]*//p' version.mk | head -n1)"
DKMS_NAME="$(sed -n 's/^DKMS_NAME[[:space:]]*:=[[:space:]]*//p' version.mk | head -n1)"

if [[ -z "${DKMS_VER:-}" || -z "${DKMS_NAME:-}" ]]; then
	echo "Failed to read version information from version.mk"
	exit 1
fi

# Get the installed version of the driver
installed_version="$(dkms status -k "$(uname -r)" | grep -oP '^(yeetmouse-driver[\/(, )]) ?\K([0-9.]+)' | head -n1 || true)"

if [[ -n "$installed_version" ]]; then
	echo "Driver ($installed_version) already installed, exiting."
	exit 0
fi

if ! getent group yeetmouse >/dev/null 2>&1; then
	echo "Creating yeetmouse group"
	sudo groupadd -r yeetmouse
fi

if ! id -nG "$TARGET_USER" | grep -qw yeetmouse; then
	echo "Adding user '$TARGET_USER' to yeetmouse group"
	sudo usermod -aG yeetmouse "$TARGET_USER"
fi

# Install files
sudo make setup_dkms
sudo make install_userspace
sudo make install_config
sudo make install_service
sudo make install_uninstaller
sudo make install_gui_optional

# Install the driver and activate the dkms module
sudo dkms install -m "$DKMS_NAME" -v "$DKMS_VER"

# Reload module if needed
sudo modprobe -r yeetmouse 2>/dev/null || true
sudo modprobe yeetmouse

if getent group yeetmouse >/dev/null 2>&1 && [[ -d /sys/module/yeetmouse/parameters ]]; then
	sudo chown root:yeetmouse /sys/module/yeetmouse/parameters/* || true
	sudo chmod 0660 /sys/module/yeetmouse/parameters/* || true
fi

# Apply config immediately if present
if [[ -f /etc/yeetmouse.conf ]]; then
	sudo /usr/bin/yeetmousectl apply /etc/yeetmouse.conf || true
fi

# Enable boot-time config apply on systemd systems
if command -v systemctl >/dev/null 2>&1 && [[ -d /run/systemd/system ]]; then
	sudo systemctl daemon-reload
	sudo systemctl enable yeetmouse.service
	sudo systemctl restart yeetmouse.service || true
else
	echo "systemd not detected; installed driver and yeetmousectl, but did not enable a boot-time service."
	echo "To persist settings across reboot on this system, run:"
	echo "  /usr/bin/yeetmousectl apply /etc/yeetmouse.conf"
	echo "from your init system's startup mechanism."
fi

echo -e "Installation complete.\n"

echo -e "\033[1m\033[1;33mIMPORTANT\033[0m"
echo -e "\033[1mYou must \033[1;31mre-login (or reboot)\033[0m\033[1m for group permissions to apply to your user session."
echo -e "For terminal testing, you can run 'newgrp yeetmouse' in a new shell.\033[0m"