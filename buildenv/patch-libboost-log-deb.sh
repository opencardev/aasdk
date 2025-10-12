#!/bin/bash
# All this because of a packaging bug in libboost-log-dev
# 'Multi-Arch: same' is not present in the control file of Debian packages, all the way up to Sid
# Rather than recompiling the parent libbost suite, download the 6 packages, extract, edit, and repackage.

# https://bugs.debian.org/cgi-bin/pkgreport.cgi?package=libboost-log-dev

# Check for required parameters
if [ $# -lt 2 ]; then
    echo "Usage: $0 <TARGET_ARCH> <BOOST_VERSION>"
    echo "Example: $0 armhf 1.83.0"
    exit 1
fi

TARGET_ARCH=$1
BOOST_VERSION=$2

echo "Patching libboost-log-dev for architecture: ${TARGET_ARCH}, version: ${BOOST_VERSION}"

# Extract version parts for package naming
BOOST_MAJOR_MINOR=$(echo ${BOOST_VERSION} | cut -d. -f1,2)
BOOST_PACKAGE_VERSION="${BOOST_VERSION}"

apt-get update

# Try to download libboost-log packages for the specified version
apt-get -y install --download-only libboost-log-dev:amd64 libboost-log${BOOST_MAJOR_MINOR}.0:amd64 libboost-log${BOOST_MAJOR_MINOR}-dev:amd64 2>/dev/null || {
    echo "Warning: Could not download some boost log packages for version ${BOOST_VERSION}"
    echo "Attempting to find available packages..."
    apt-cache search libboost-log | grep -E "libboost-log[0-9]+\.[0-9]+" | head -5
}

if [ "${TARGET_ARCH}" != "amd64" ]; then
    apt-get -y install --download-only libboost-log-dev:${TARGET_ARCH} libboost-log${BOOST_MAJOR_MINOR}.0:${TARGET_ARCH} libboost-log${BOOST_MAJOR_MINOR}-dev:${TARGET_ARCH} 2>/dev/null || {
        echo "Warning: Could not download boost log packages for ${TARGET_ARCH}"
    }
fi

# Find the actual downloaded packages
CACHE_DIR="/var/cache/apt/archives"
LIBBOOST_LOG_DEV_AMD64=$(ls ${CACHE_DIR}/libboost-log-dev_*_amd64.deb 2>/dev/null | head -1)
LIBBOOST_LOG_DEV_ARCH=$(ls ${CACHE_DIR}/libboost-log-dev_*_${TARGET_ARCH}.deb 2>/dev/null | head -1)

if [ -z "${LIBBOOST_LOG_DEV_AMD64}" ] && [ -z "${LIBBOOST_LOG_DEV_ARCH}" ]; then
    echo "No libboost-log-dev packages found to patch. Skipping patch."
    exit 0
fi

echo "Found packages to patch:"
[ -n "${LIBBOOST_LOG_DEV_AMD64}" ] && echo "  AMD64: $(basename ${LIBBOOST_LOG_DEV_AMD64})"
[ -n "${LIBBOOST_LOG_DEV_ARCH}" ] && echo "  ${TARGET_ARCH}: $(basename ${LIBBOOST_LOG_DEV_ARCH})"

mkdir -p /tmp/armhf/libboost-log-dev/
dpkg-deb -x /var/cache/apt/archives/libboost-log-dev_1.67.0.1_armhf.deb /tmp/armhf/libboost-log-dev/
dpkg-deb -e /var/cache/apt/archives/libboost-log-dev_1.67.0.1_armhf.deb /tmp/armhf/libboost-log-dev/DEBIAN/
sed -i '/^Priority: optional/a Multi-Arch: same' /tmp/armhf/libboost-log-dev/DEBIAN/control
dpkg-deb -b /tmp/armhf/libboost-log-dev/ /tmp/libboost-log-dev_1.67.0.1_armhf.deb

mkdir -p /tmp/amd64/libboost-log-dev/
dpkg-deb -x /var/cache/apt/archives/libboost-log-dev_1.67.0.1_amd64.deb /tmp/amd64/libboost-log-dev/
dpkg-deb -e /var/cache/apt/archives/libboost-log-dev_1.67.0.1_amd64.deb /tmp/amd64/libboost-log-dev/DEBIAN/
sed -i '/^Priority: optional/a Multi-Arch: same' /tmp/amd64/libboost-log-dev/DEBIAN/control
dpkg-deb -b /tmp/amd64/libboost-log-dev/ /tmp/libboost-log-dev_1.67.0.1_amd64.deb

mkdir -p /tmp/armhf/libboost-log1.67-dev/
dpkg-deb -x /var/cache/apt/archives/libboost-log1.67-dev_1.67.0-13+deb10u1_armhf.deb /tmp/armhf/libboost-log1.67-dev/
dpkg-deb -e /var/cache/apt/archives/libboost-log1.67-dev_1.67.0-13+deb10u1_armhf.deb /tmp/armhf/libboost-log1.67-dev/DEBIAN/
sed -i '/^Priority: optional/a Multi-Arch: same' /tmp/armhf/libboost-log1.67-dev/DEBIAN/control
dpkg-deb -b /tmp/armhf/libboost-log1.67-dev/ /tmp/libboost-log1.67-dev_1.67.0-13+deb10u1_armhf.deb

mkdir -p /tmp/amd64/libboost-log1.67-dev/
dpkg-deb -x /var/cache/apt/archives/libboost-log1.67-dev_1.67.0-13+deb10u1_amd64.deb /tmp/amd64/libboost-log1.67-dev/
dpkg-deb -e /var/cache/apt/archives/libboost-log1.67-dev_1.67.0-13+deb10u1_amd64.deb /tmp/amd64/libboost-log1.67-dev/DEBIAN/
sed -i '/^Priority: optional/a Multi-Arch: same' /tmp/amd64/libboost-log1.67-dev/DEBIAN/control
dpkg-deb -b /tmp/amd64/libboost-log1.67-dev/ /tmp/libboost-log1.67-dev_1.67.0-13+deb10u1_amd64.deb

mkdir -p /tmp/armhf/libboost-log1.67.0/
dpkg-deb -x /var/cache/apt/archives/libboost-log1.67.0_1.67.0-13+deb10u1_armhf.deb /tmp/armhf/libboost-log1.67.0/
dpkg-deb -e /var/cache/apt/archives/libboost-log1.67.0_1.67.0-13+deb10u1_armhf.deb /tmp/armhf/libboost-log1.67.0/DEBIAN/
sed -i '/^Priority: optional/a Multi-Arch: same' /tmp/armhf/libboost-log1.67.0/DEBIAN/control
dpkg-deb -b /tmp/armhf/libboost-log1.67.0/ /tmp/libboost-log1.67.0_1.67.0-13+deb10u1_armhf.deb

mkdir -p /tmp/amd64/libboost-log1.67.0/
dpkg-deb -x /var/cache/apt/archives/libboost-log1.67.0_1.67.0-13+deb10u1_amd64.deb /tmp/amd64/libboost-log1.67.0/
dpkg-deb -e /var/cache/apt/archives/libboost-log1.67.0_1.67.0-13+deb10u1_amd64.deb /tmp/amd64/libboost-log1.67.0/DEBIAN/
sed -i '/^Priority: optional/a Multi-Arch: same' /tmp/amd64/libboost-log1.67.0/DEBIAN/control
dpkg-deb -b /tmp/amd64/libboost-log1.67.0/ /tmp/libboost-log1.67.0_1.67.0-13+deb10u1_amd64.deb

dpkg -i /tmp/libboost*.deb