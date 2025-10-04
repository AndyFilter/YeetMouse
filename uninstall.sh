#!/bin/bash

VERSION=$(grep -Po '(?<=DKMS_VER\?=)[0-9\.]+' "Makefile")

# Get the installed version of the driver
installed_version=$(dkms status -k $(uname -r) | grep -oP '^([l|y]eetmouse-driver[\/(, )]) ?\K([0-9.]+)')

# Check if the driver is even installed
if [[ -z "$installed_version" ]]; then
	echo "Driver not installed, exiting."
	exit 0
fi

# Check if the installed version is old
if [[ $(printf '%s\n' "$installed_version" "$VERSION" | sort -V | head -n1) != "$VERSION" ]]; then
	echo "Please uninstall the old driver ($installed_version) using the old uninstaller first!"
	exit 1
fi

# Uninstall the driver
#sudo dkms remove -m yeetmouse-driver -v $installed_version # Newer dkms versions
sudo dkms remove "yeetmouse-driver/$installed_version" --all # Older (for Ubuntu <= 20.04), but should work now too
sudo make remove_dkms
sudo modprobe -r yeetmouse
sudo rmmod yeetmouse
