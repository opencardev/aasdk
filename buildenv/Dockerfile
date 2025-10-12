# syntax=docker/dockerfile:1

ARG DEBIAN_RELEASE=trixie
ARG TARGET_ARCH=armhf
ARG BOOST_VERSION=1.83.0

FROM debian:${DEBIAN_RELEASE} AS base

# Install base dependencies
RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

FROM base AS build-tools

# Install build dependencies
RUN <<EOF
apt-get update
apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    git \
    pkg-config \
    protobuf-compiler \
    python3 \
    python3-dev
apt-get clean
rm -rf /var/lib/apt/lists/*
EOF

FROM build-tools AS cross-compiler

ARG TARGET_ARCH

# Add target architecture and install cross-compilation tools
RUN <<EOF
dpkg --add-architecture ${TARGET_ARCH}
apt-get update

# Install cross-compilation toolchains based on architecture
case "${TARGET_ARCH}" in
    armhf)
        apt-get install -y --no-install-recommends \
            gcc-arm-linux-gnueabihf \
            g++-arm-linux-gnueabihf \
            cpp-arm-linux-gnueabihf
        ;;
    arm64)
        apt-get install -y --no-install-recommends \
            gcc-aarch64-linux-gnu \
            g++-aarch64-linux-gnu \
            cpp-aarch64-linux-gnu
        ;;
    amd64)
        echo "Native compilation for amd64"
        ;;
    *)
        echo "Unsupported architecture: ${TARGET_ARCH}" >&2
        exit 1
        ;;
esac

apt-get clean
rm -rf /var/lib/apt/lists/*
EOF

FROM cross-compiler AS dependencies

ARG TARGET_ARCH
ARG BOOST_VERSION

# Install target-specific dependencies
RUN <<EOF
apt-get update

# Install base libraries for target architecture
apt-get install -y --no-install-recommends \
    libprotobuf-dev:${TARGET_ARCH} \
    libusb-1.0-0-dev:${TARGET_ARCH} \
    libssl-dev:${TARGET_ARCH} \
    libboost-dev:${TARGET_ARCH} \
    libboost-system-dev:${TARGET_ARCH}

# Install specific boost libraries using the version parameter
apt-get install -y --no-install-recommends \
    libboost-atomic${BOOST_VERSION}:${TARGET_ARCH} \
    libboost-chrono${BOOST_VERSION}:${TARGET_ARCH} \
    libboost-date-time${BOOST_VERSION}:${TARGET_ARCH} \
    libboost-filesystem${BOOST_VERSION}:${TARGET_ARCH} \
    libboost-regex${BOOST_VERSION}:${TARGET_ARCH} \
    libboost-system${BOOST_VERSION}:${TARGET_ARCH} \
    libboost-thread${BOOST_VERSION}:${TARGET_ARCH} \
    libboost-filesystem$(echo ${BOOST_VERSION} | cut -d. -f1,2)-dev:${TARGET_ARCH} \
    libboost-thread$(echo ${BOOST_VERSION} | cut -d. -f1,2)-dev:${TARGET_ARCH} \
    libboost-date-time$(echo ${BOOST_VERSION} | cut -d. -f1,2)-dev:${TARGET_ARCH}

apt-get clean
rm -rf /var/lib/apt/lists/*
EOF

FROM dependencies AS builder

ARG TARGET_ARCH
ARG BOOST_VERSION

# Set up environment variables for cross-compilation
ENV TARGET_ARCH=${TARGET_ARCH}
ENV DEBIAN_FRONTEND=noninteractive

# Copy source code
COPY --chown=root:root . /src
WORKDIR /src

# Copy build scripts with proper permissions
COPY --chmod=755 ./buildenv/patch-libboost-log-deb.sh /src/resources/patch-libboost-log-deb.sh
COPY --chmod=755 ./buildenv/entrypoint.sh /entrypoint.sh

# Detect the actual installed boost version and apply boost log patch
RUN <<EOF
# Try to detect the actual installed boost version from installed packages
INSTALLED_BOOST_VERSION=$(dpkg -l | grep libboost-system | grep "${TARGET_ARCH}" | awk '{print $3}' | cut -d'-' -f1 | head -1)

if [ -z "${INSTALLED_BOOST_VERSION}" ]; then
    echo "Could not detect installed boost version, using build argument: ${BOOST_VERSION}"
    INSTALLED_BOOST_VERSION="${BOOST_VERSION}"
else
    echo "Detected installed boost version: ${INSTALLED_BOOST_VERSION}"
fi

# Run the patch script with detected version
/src/resources/patch-libboost-log-deb.sh ${TARGET_ARCH} ${INSTALLED_BOOST_VERSION}
EOF

# Set up cross-compilation environment
RUN <<EOF
case "${TARGET_ARCH}" in
    armhf)
        {
            echo "export CC=arm-linux-gnueabihf-gcc"
            echo "export CXX=arm-linux-gnueabihf-g++"
            echo "export AR=arm-linux-gnueabihf-ar"
            echo "export STRIP=arm-linux-gnueabihf-strip"
            echo "export PKG_CONFIG_PATH=/usr/lib/arm-linux-gnueabihf/pkgconfig"
        } >> /etc/environment
        ;;
    arm64)
        {
            echo "export CC=aarch64-linux-gnu-gcc"
            echo "export CXX=aarch64-linux-gnu-g++"
            echo "export AR=aarch64-linux-gnu-ar"
            echo "export STRIP=aarch64-linux-gnu-strip"
            echo "export PKG_CONFIG_PATH=/usr/lib/aarch64-linux-gnu/pkgconfig"
        } >> /etc/environment
        ;;
esac
EOF

ENTRYPOINT ["/bin/bash", "/entrypoint.sh"]