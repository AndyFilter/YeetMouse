#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
INSTALLED_UNINSTALLER="/usr/bin/yeetmouse-uninstall"
REPO_UNINSTALLER="${SCRIPT_DIR}/install_files/yeetmouse-uninstall.sh"

if [[ -x "$INSTALLED_UNINSTALLER" ]]; then
	echo "Using installed uninstaller: $INSTALLED_UNINSTALLER"
	exec sudo "$INSTALLED_UNINSTALLER" "$@"
fi

if [[ -x "$REPO_UNINSTALLER" ]]; then
	echo "Installed uninstaller not found, using repository uninstaller: $REPO_UNINSTALLER"
	exec sudo "$REPO_UNINSTALLER" "$@"
fi

echo "No usable YeetMouse uninstaller was found."
echo "Expected one of:"
echo "  $INSTALLED_UNINSTALLER"
echo "  $REPO_UNINSTALLER"
exit 1