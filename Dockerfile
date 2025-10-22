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

FROM debian:trixie-slim

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

    # Build script that handles architecture-specific builds  
    RUN echo "Building AASDK for architecture: native compilation" && \
        export TARGET_ARCH=$(dpkg-architecture -qDEB_HOST_ARCH) && \
        echo "Detected architecture: $TARGET_ARCH" && \
        export CROSS_COMPILE=false && \
        export CC=/usr/bin/cc && \
        export CXX=/usr/bin/c++ && \
        echo "Cross-compilation: $CROSS_COMPILE" && \
        echo "CC: $CC, CXX: $CXX" && \
        echo "Multi-arch path: $(dpkg-architecture -qDEB_HOST_MULTIARCH)" && \
        export JOBS=$(nproc) && \
        export PKG_CONFIG_PATH="/usr/lib/pkgconfig:/usr/lib/$(dpkg-architecture -qDEB_HOST_MULTIARCH)/pkgconfig" && \
        export CMAKE_PREFIX_PATH=/usr && \
        export CMAKE_ARGS="-DCMAKE_C_COMPILER=/usr/bin/cc -DCMAKE_CXX_COMPILER=/usr/bin/c++ -DProtobuf_INCLUDE_DIR=/usr/include -DProtobuf_LIBRARIES=/usr/lib/$(dpkg-architecture -qDEB_HOST_MULTIARCH)/libprotobuf.so -DProtobuf_LIBRARY=/usr/lib/$(dpkg-architecture -qDEB_HOST_MULTIARCH)/libprotobuf.so -DProtobuf_LITE_LIBRARY=/usr/lib/$(dpkg-architecture -qDEB_HOST_MULTIARCH)/libprotobuf-lite.so -DProtobuf_PROTOC_EXECUTABLE=/usr/bin/protoc -DLIBUSB_1_INCLUDE_DIRS=/usr/include/libusb-1.0 -DLIBUSB_1_LIBRARIES=/usr/lib/$(dpkg-architecture -qDEB_HOST_MULTIARCH)/libusb-1.0.so -DOPENSSL_INCLUDE_DIR=/usr/include/openssl -DOPENSSL_CRYPTO_LIBRARY=/usr/lib/$(dpkg-architecture -qDEB_HOST_MULTIARCH)/libcrypto.so -DOPENSSL_SSL_LIBRARY=/usr/lib/$(dpkg-architecture -qDEB_HOST_MULTIARCH)/libssl.so" && \
        echo "Protobuf check:" && \
        pkg-config --exists protobuf && echo "✅ protobuf found via pkg-config" || echo "❌ protobuf not found via pkg-config" && \
        pkg-config --cflags protobuf && \
        pkg-config --libs protobuf && \
        echo "libusb check:" && \
        pkg-config --exists libusb-1.0 && echo "✅ libusb-1.0 found via pkg-config" || echo "❌ libusb-1.0 not found via pkg-config" && \
        pkg-config --cflags libusb-1.0 && \
        pkg-config --libs libusb-1.0 && \
        echo "OpenSSL check:" && \
        pkg-config --exists openssl && echo "✅ openssl found via pkg-config" || echo "❌ openssl not found via pkg-config" && \
        pkg-config --cflags openssl && \
        pkg-config --libs openssl && \
        echo "Environment variables:" && \
        echo "PKG_CONFIG_PATH=$PKG_CONFIG_PATH" && \
        echo "CMAKE_PREFIX_PATH=$CMAKE_PREFIX_PATH" && \
        echo "CMAKE_ARGS=$CMAKE_ARGS" && \
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