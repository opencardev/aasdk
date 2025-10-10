# syntax=docker/dockerfile:1

ARG DEBIAN_RELEASE=trixie
ARG TARGET_ARCH=armhf

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

# Install specific boost libraries for Debian Trixie
apt-get install -y --no-install-recommends \
    libboost-atomic1.83.0:${TARGET_ARCH} \
    libboost-chrono1.83.0:${TARGET_ARCH} \
    libboost-date-time1.83.0:${TARGET_ARCH} \
    libboost-filesystem1.83.0:${TARGET_ARCH} \
    libboost-regex1.83.0:${TARGET_ARCH} \
    libboost-system1.83.0:${TARGET_ARCH} \
    libboost-thread1.83.0:${TARGET_ARCH} \
    libboost-filesystem1.83-dev:${TARGET_ARCH} \
    libboost-thread1.83-dev:${TARGET_ARCH} \
    libboost-date-time1.83-dev:${TARGET_ARCH}

apt-get clean
rm -rf /var/lib/apt/lists/*
EOF

FROM dependencies AS builder

ARG TARGET_ARCH

# Set up environment variables for cross-compilation
ENV TARGET_ARCH=${TARGET_ARCH}
ENV DEBIAN_FRONTEND=noninteractive

# Copy source code
COPY --chown=root:root . /src
WORKDIR /src

# Copy build scripts with proper permissions
COPY --chmod=755 ./buildenv/patch-libboost-log-deb.sh /src/resources/patch-libboost-log-deb.sh
COPY --chmod=755 ./buildenv/entrypoint.sh /entrypoint.sh

# Apply boost log patch
RUN /src/resources/patch-libboost-log-deb.sh ${TARGET_ARCH}

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