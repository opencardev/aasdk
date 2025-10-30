# * Project: OpenAuto
# * This file is part of openauto project.
# * Copyright (C) 2025 OpenCarDev Team
# *
# *  openauto is free software: you can redistribute it and/or modify
# *  it under the terms of the GNU General Public License as published by
# *  the Free Software Foundation; either version 3 of the License, or
# *  (at your option) any later version.
# *
# *  openauto is distributed in the hope that it will be useful,
# *  but WITHOUT ANY WARRANTY; without even the implied warranty of
# *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# *  GNU General Public License for more details.
# *
# *  You should have received a copy of the GNU General Public License
# *  along with openauto. If not, see <http://www.gnu.org/licenses/>.

# Multi-stage build for AASDK with native compilation for each architecture
# This Dockerfile builds AASDK natively on each target platform using QEMU emulation

# Allow selecting Debian base (bookworm or trixie). Default to trixie.
ARG DEBIAN_VERSION=trixie
FROM debian:${DEBIAN_VERSION}-slim

# Build arguments
ARG TARGET_ARCH=amd64
ARG DEBIAN_FRONTEND=noninteractive
ARG GIT_COMMIT_ID=unknown
ARG GIT_BRANCH=unknown
ARG GIT_DESCRIBE=unknown
ARG GIT_DIRTY=unknown

# Export git info as environment variables
ENV GIT_COMMIT_ID=${GIT_COMMIT_ID}
ENV GIT_BRANCH=${GIT_BRANCH}
ENV GIT_DESCRIBE=${GIT_DESCRIBE}
ENV GIT_DIRTY=${GIT_DIRTY}

# Debug: Print git variables
RUN echo "Docker build args received:" && \
    echo "  GIT_COMMIT_ID=${GIT_COMMIT_ID}" && \
    echo "  GIT_BRANCH=${GIT_BRANCH}" && \
    echo "  GIT_DESCRIBE=${GIT_DESCRIBE}" && \
    echo "  GIT_DIRTY=${GIT_DIRTY}"

# Set locale to avoid encoding issues
ENV LANG=C.UTF-8
ENV LC_ALL=C.UTF-8

# Install build dependencies and tools
RUN apt-get update && apt-get install -y \
    # Core build tools
    build-essential \
    cmake \
    ninja-build \
    pkg-config \
    git \
    # Development libraries (native for this platform)
    libboost-all-dev \
    libprotobuf-dev \
    protobuf-compiler \
    libusb-1.0-0-dev \
    libssl-dev \
    # Packaging tools
    file \
    dpkg-dev \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /src

# Copy source code.
COPY . .

# Debug: List what was copied
RUN echo "Contents of /src:" && ls -la

# Create output directory for packages
RUN mkdir -p /output

# Make build script executable and verify it exists
RUN if [ -f "build.sh" ]; then \
        echo "Found build.sh, making executable"; \
        chmod +x build.sh; \
    else \
        echo "ERROR: build.sh not found!"; \
        echo "Current directory contents:"; \
        ls -la; \
        exit 1; \
    fi

# Build and package AASDK using the simplified build.sh approach
ARG CROSS_COMPILE=false
RUN echo "=== Git environment variables before build ===" && \
    echo "  GIT_COMMIT_ID=$GIT_COMMIT_ID" && \
    echo "  GIT_BRANCH=$GIT_BRANCH" && \
    echo "  GIT_DESCRIBE=$GIT_DESCRIBE" && \
    echo "  GIT_DIRTY=$GIT_DIRTY" && \
    echo "=============================================" && \
    export TARGET_ARCH=$(dpkg-architecture -qDEB_HOST_ARCH) && \
        echo "Building AASDK for architecture: $TARGET_ARCH (native compilation)" && \
        # Compute distro-specific release suffix for DEB packages
        DISTRO_DEB_RELEASE=$(bash /src/scripts/distro_release.sh) && \
        CPACK_DEB_RELEASE="$DISTRO_DEB_RELEASE" && \
        echo "Using CPACK_DEBIAN_PACKAGE_RELEASE: $CPACK_DEB_RELEASE" && \
        # Pass through to CMake via build.sh using CMAKE_ARGS
        env DISTRO_DEB_RELEASE="$CPACK_DEB_RELEASE" CMAKE_ARGS="$CMAKE_ARGS -DCPACK_DEBIAN_PACKAGE_RELEASE=$CPACK_DEB_RELEASE -DCPACK_PROJECT_CONFIG_FILE=/src/cmake_modules/CPackProjectConfig.cmake" \
            CROSS_COMPILE=${CROSS_COMPILE} \
            ./build.sh release clean package && \
    if [ -d "packages" ]; then \
        cp packages/*.deb /output/ 2>/dev/null || true && \
        echo "Packages built:" && \
        ls -la /output/; \
    else \
        echo "No packages directory found" && \
        find . -name "*.deb" -exec cp {} /output/ \; 2>/dev/null || true; \
    fi && \
    echo "Build completed"# Default command
CMD ["bash", "-c", "echo 'AASDK build container ready. Use docker run with volume mounts to build packages.'"]