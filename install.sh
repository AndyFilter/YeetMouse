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

require_command() {
	if ! command -v "$1" >/dev/null 2>&1; then
		echo "Missing required command: $1"
		exit 1
	fi
}

rollback() {
	local exit_code=$?
	if [[ $exit_code -eq 0 ]]; then
		return
	fi

	echo
	echo -e "\033[1m\033[1;33mInstallation failed.\033[0m \033[1mAttempting rollback of changes from this run...\033[0m"

	if [[ "${DKMS_MODULE_INSTALLED:-0}" -eq 1 ]]; then
		sudo dkms remove "${DKMS_NAME}/${DKMS_VER}" --all 2>/dev/null || true
	fi

	if [[ "${INSTALLED_SERVICE:-0}" -eq 1 ]]; then
		if command -v systemctl >/dev/null 2>&1 && [[ -d /run/systemd/system ]]; then
			sudo systemctl disable --now yeetmouse.service 2>/dev/null || true
			sudo systemctl daemon-reload || true
		fi
		sudo make remove_service || true
	fi

	if [[ "${INSTALLED_GUI:-0}" -eq 1 ]]; then
		sudo make remove_gui || true
	fi

	if [[ "${INSTALLED_UNINSTALLER:-0}" -eq 1 ]]; then
		sudo make remove_uninstaller || true
	fi

	if [[ "${INSTALLED_USERSPACE:-0}" -eq 1 ]]; then
		sudo make remove_userspace || true
	fi

	if [[ "${INSTALLED_DKMS_FILES:-0}" -eq 1 ]]; then
		sudo make remove_dkms || true
	fi

	sudo modprobe -r yeetmouse 2>/dev/null || true
	sudo rmmod yeetmouse 2>/dev/null || true

	echo -e "\033[1mRollback finished.\033[0m"
	echo "Note: the yeetmouse group and user-group membership were not reverted."
	echo "You may run 'sudo yeetmouse-uninstall' later if needed."
	exit "$exit_code"
}

trap rollback ERR

require_command sudo
require_command make
require_command dkms
require_command modprobe
require_command groupadd
require_command usermod
require_command getent

if [[ ! -d "/lib/modules/$(uname -r)/build" ]]; then
	echo "Kernel headers/build directory not found for kernel: $(uname -r)"
	echo "Expected directory: /lib/modules/$(uname -r)/build"
	echo "Please install the matching kernel headers first."
	exit 1
fi

# Get the installed version of the driver
installed_version="$(dkms status | grep -oP '^(yeetmouse-driver)[\/, ]+\K([0-9.]+)' | head -n1 || true)"

if [[ -n "$installed_version" ]]; then
	echo "Driver ($installed_version) already installed, exiting."
	echo "If this is a broken or partial install, run: sudo yeetmouse-uninstall"
	exit 0
fi

CREATED_GROUP=0
INSTALLED_DKMS_FILES=0
INSTALLED_USERSPACE=0
INSTALLED_SERVICE=0
INSTALLED_UNINSTALLER=0
INSTALLED_GUI=0
DKMS_MODULE_INSTALLED=0

if ! getent group yeetmouse >/dev/null 2>&1; then
	echo "Creating yeetmouse group"
	sudo groupadd -r yeetmouse
	CREATED_GROUP=1
fi

if ! id -nG "$TARGET_USER" | grep -qw yeetmouse; then
	echo "Adding user '$TARGET_USER' to yeetmouse group"
	sudo usermod -aG yeetmouse "$TARGET_USER"
fi

INSTALLED_DKMS_FILES=1
sudo make setup_dkms

INSTALLED_USERSPACE=1
sudo make install_userspace

sudo make install_config

INSTALLED_SERVICE=1
sudo make install_service

INSTALLED_UNINSTALLER=1
sudo make install_uninstaller

if sudo make install_gui_optional; then
	if [[ -x /usr/bin/yeetmouse ]]; then
		INSTALLED_GUI=1
	fi
fi

DKMS_MODULE_INSTALLED=1
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

trap - ERR

echo -e "Installation complete.\n"

echo -e "\033[1m\033[1;33mIMPORTANT\033[0m"
echo -e "\033[1mYou must \033[1;31mre-login (or reboot)\033[0m\033[1m for group permissions to apply to your user session."
echo -e "For terminal testing, you can run 'newgrp yeetmouse' in a new shell.\033[0m"