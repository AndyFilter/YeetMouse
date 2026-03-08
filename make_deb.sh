#!/usr/bin/env bash
set -euo pipefail

PKGNAME=yeetmouse
ROOT="$(git rev-parse --show-toplevel)"
VERSION="$(grep -Po '^\s*DKMS_VER\??=\s*\K[0-9.]+$' "${ROOT}/Makefile")"
ARCH=amd64
BUILD_DIR="${ROOT}/build_deb"
PKG_ROOT="${BUILD_DIR}/${PKGNAME}_${VERSION}_${ARCH}"
DKMS_NAME="yeetmouse-driver"
DKMS_SRC_DIR="${PKG_ROOT}/usr/src/${DKMS_NAME}-${VERSION}"

echo "Building ${PKGNAME} ${VERSION} (${ARCH})..."

# Clean
rm -rf "${BUILD_DIR}"
mkdir -p "${PKG_ROOT}"

# Build GUI
#make -C "${ROOT}/gui" clean # COMMENTED OUT FOR TESTING PURPOSES AND TO SAVE POWER
make -C "${ROOT}/gui" -j

# Layout: DKMS sources at /usr/src/${DKMS_NAME}-${VERSION}
mkdir -p "${DKMS_SRC_DIR}/driver/FixedMath"

# Core files
install -m 0644 "${ROOT}/Makefile" "${DKMS_SRC_DIR}/Makefile"
install -m 0644 "${ROOT}/shared_definitions.h" "${DKMS_SRC_DIR}/shared_definitions.h"

# Driver sources
install -m 0644 "${ROOT}/driver/Makefile" "${DKMS_SRC_DIR}/driver/Makefile"
install -m 0644 "${ROOT}/driver/"*.c "${DKMS_SRC_DIR}/driver/"
install -m 0644 "${ROOT}/driver/"*.h "${DKMS_SRC_DIR}/driver/"
install -m 0644 "${ROOT}/driver/FixedMath/"*.h "${DKMS_SRC_DIR}/driver/FixedMath/"

# DKMS config
install -m 0644 "${ROOT}/install_files/dkms/dkms.conf" "${DKMS_SRC_DIR}/dkms.conf"

# GUI binary
install -d "${PKG_ROOT}/usr/bin"
install -m 0755 "${ROOT}/gui/YeetMouseGui" "${PKG_ROOT}/usr/bin/YeetMouse"

# Control files
mkdir -p "${PKG_ROOT}/DEBIAN"
sed "s/__VERSION__/${VERSION}/g" "${ROOT}/packaging/deb/control" > "${PKG_ROOT}/DEBIAN/control"
sed "s/__VERSION__/${VERSION}/g" "${ROOT}/packaging/deb/postinst" > "${PKG_ROOT}/DEBIAN/postinst"
sed "s/__VERSION__/${VERSION}/g" "${ROOT}/packaging/deb/prerm" > "${PKG_ROOT}/DEBIAN/prerm"

# Set permissions
chmod 0755 "${PKG_ROOT}/DEBIAN/postinst" "${PKG_ROOT}/DEBIAN/prerm"
chmod 0644 "${PKG_ROOT}/DEBIAN/control"

# Build the .deb
dpkg-deb --build "${PKG_ROOT}"

echo "Built package at ${BUILD_DIR}/${PKGNAME}_${VERSION}_${ARCH}.deb"