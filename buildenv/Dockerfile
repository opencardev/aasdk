# syntax=docker/dockerfile:1

ARG DEBIAN_RELEASE=trixie
ARG TARGET_ARCH=armhf

FROM debian:${DEBIAN_RELEASE} AS base

# Set up environment variables
ENV DEBIAN_FRONTEND=noninteractive

# Install base dependencies
RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

FROM base AS build-tools

# Set up environment variables
ENV DEBIAN_FRONTEND=noninteractive

# Install build dependencies for native compilation
RUN <<EOF
apt-get update
apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    git \
    pkg-config \
    protobuf-compiler \
    python3 \
    python3-dev \
    dpkg-dev \
    fakeroot
apt-get clean
rm -rf /var/lib/apt/lists/*
EOF

FROM build-tools AS dependencies

# Set up environment variables
ENV DEBIAN_FRONTEND=noninteractive

# Install native dependencies (no cross-compilation needed)
RUN <<EOF
apt-get update

# Install all required libraries natively
apt-get install -y --no-install-recommends \
    libprotobuf-dev \
    libusb-1.0-0-dev \
    libssl-dev \
    libboost-all-dev

apt-get clean
rm -rf /var/lib/apt/lists/*
EOF

FROM dependencies AS builder

ARG TARGET_ARCH

# Set up environment variables
ENV TARGET_ARCH=${TARGET_ARCH}
ENV DEBIAN_FRONTEND=noninteractive

# Copy source code
COPY --chown=root:root . /src
WORKDIR /src

# Copy build script with proper permissions
COPY --chmod=755 ./buildenv/entrypoint.sh /entrypoint.sh

# No need for boost patch since we're using native compilation
# libboost-all-dev provides all required boost libraries without Multi-Arch issues

ENTRYPOINT ["/bin/bash", "/entrypoint.sh"]