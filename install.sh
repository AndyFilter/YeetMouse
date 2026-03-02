#!/bin/bash

# Get the installed version of the driver
installed_version=$(dkms status -k $(uname -r) | grep -oP '^([l|y]eetmouse-driver[\/(, )]) ?\K([0-9.]+)')

if [[ ! -z "$installed_version" ]]; then
	echo "Driver ($installed_version) already installed, exiting."
	exit 0
fi

# Try to fix config.h saved in old format (acceleration mode number)
CONFIG_FILE="driver/config.h"
ENUM_FILE="shared_definitions.h"

# Extract the number from the config file
mode_number=$(grep -Po '^#define\s+ACCELERATION_MODE\s+\K\d+' "$CONFIG_FILE")

if [[ "$mode_number" ]]; then

	echo "Fixing old version of config..."

	old_mode_numer=$mode_number

	if [ $mode_number -gt 4 ]; then
		mode_number=$((mode_number+2))
	fi

	# Find matching enum entry from shared_definitions.h
	mode_name=$(grep -Po "AccelMode_[A-Za-z0-9_]+\s*=\s*$mode_number\b" "$ENUM_FILE" | awk -F= '{print $1}' | tr -d '[:space:]')

	if [[ -z "$mode_name" ]]; then
		  echo "Could not find enum entry for value $mode_number in $ENUM_FILE"
		  exit 1
	fi

	# Replace number in config.h with the enum name
	sudo sed -i -E "s|(#define\s+ACCELERATION_MODE\s+)$old_mode_numer|\1$mode_name|" "$CONFIG_FILE"

	echo "Done! Replaced ACCELERATION_MODE $old_mode_numer with $mode_name in $CONFIG_FILE"

fi

# Install the driver and activate the dkms module
sudo make setup_dkms
sudo dkms install -m yeetmouse-driver -v 0.9.2 # Enter the version you determined from the Makefile earlier in here
sudo modprobe yeetmouse
