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

# Multi-architecture AASDK build container using native compilation
ARG DEBIAN_VERSION=trixie
FROM debian:${DEBIAN_VERSION}

# Build arguments
ARG TARGET_ARCH=amd64
ARG DEBIAN_FRONTEND=noninteractive

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

# Copy source code
COPY . .

# Create output directory for packages
RUN mkdir -p /output

# Build script that handles architecture-specific builds
RUN <<EOF
#!/bin/bash
set -e

echo "Building AASDK for architecture: ${TARGET_ARCH}"

# Set environment variables for the build
export TARGET_ARCH=${TARGET_ARCH}
export JOBS=$(nproc)

# Make build script executable
chmod +x build.sh

# Run the build with package creation
./build.sh release clean package

# Move packages to output directory
if [ -d "packages" ]; then
    cp packages/*.deb /output/ 2>/dev/null || true
    echo "Packages built:"
    ls -la /output/
else
    echo "No packages directory found"
    # Look for .deb files in build directories
    find . -name "*.deb" -exec cp {} /output/ \; 2>/dev/null || true
fi

echo "Build completed for ${TARGET_ARCH}"
EOF

# Default command
CMD ["bash", "-c", "echo 'AASDK build container ready. Use docker run with volume mounts to build packages.'"]