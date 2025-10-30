#!/bin/bash

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

set -e
set -u

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

print_header() {
    echo -e "${BLUE}================================================${NC}"
    echo -e "${BLUE}  AASDK Docker/Podman Build Script${NC}"
    echo -e "${BLUE}================================================${NC}"
}

print_step() {
    echo -e "${YELLOW}🔄 $1${NC}"
}

print_success() {
    echo -e "${GREEN}✅ $1${NC}"
}

print_error() {
    echo -e "${RED}❌ $1${NC}"
}

show_usage() {
    echo "AASDK Docker/Podman Build Script"
    echo
    echo "Usage: $0 [ARCHITECTURE] [OPTIONS]"
    echo
    echo "ARCHITECTURE:"
    echo "  amd64       Build for AMD64/x86_64 (default)"
    echo "  arm64       Build for ARM64/aarch64"
    echo "  armhf       Build for ARMHF/armv7"
    echo "  all         Build for all architectures"
    echo
    echo "OPTIONS:"
    echo "  --clean     Clean build output directory before building"
    echo "  --no-cache  Don't use build cache"
    echo "  --podman    Use Podman instead of Docker"
    echo "  --help      Show this help message"
    echo
    echo "Examples:"
    echo "  $0 amd64           # Build for AMD64"
    echo "  $0 arm64 --clean   # Clean build for ARM64"
    echo "  $0 all --podman    # Build for all architectures using Podman"
    echo
}

# Collect git details
GIT_COMMIT_ID="$(git rev-parse --short HEAD 2>/dev/null || echo unknown)"
GIT_BRANCH="$(git rev-parse --abbrev-ref HEAD 2>/dev/null || echo unknown)"
GIT_DESCRIBE="$(git describe --tags --dirty --always 2>/dev/null || echo unknown)"
GIT_DIRTY="$(git diff --quiet 2>/dev/null; echo $?)"

# Container engine selection
CONTAINER_CMD="docker"
USE_PODMAN=false

ARCH="amd64"
CLEAN=false
NO_CACHE=""

for arg in "$@"; do
    case $arg in
        --clean)
            CLEAN=true
            ;;
        --no-cache)
            NO_CACHE="--no-cache"
            ;;
        --podman)
            USE_PODMAN=true
            ;;
        --help|-h)
            show_usage
            exit 0
            ;;
        amd64|arm64|armhf|all)
            ARCH=$arg
            ;;
    esac
done

if [ "$USE_PODMAN" = true ]; then
    if command -v podman &> /dev/null; then
        CONTAINER_CMD="podman"
        print_step "Using Podman as container engine."
    else
        print_error "Podman requested but not installed."
        exit 1
    fi
else
    if ! command -v docker &> /dev/null; then
        if command -v podman &> /dev/null; then
            CONTAINER_CMD="podman"
            print_step "Docker not found, falling back to Podman."
        else
            print_error "Neither Docker nor Podman is installed."
            exit 1
        fi
    fi
fi

print_header

# Check if Buildx is available (only for Docker)
if [ "$CONTAINER_CMD" = "docker" ]; then
    if ! docker buildx version &> /dev/null; then
        print_error "Docker Buildx is not available"
        exit 1
    fi
fi

build_architecture() {
    local arch=$1
    local no_cache_flag=$2

    print_step "Building AASDK for ${arch}..."

    mkdir -p build-output

    local build_args=""
    if [ "$no_cache_flag" = "--no-cache" ]; then
        build_args="--no-cache"
    fi

    # Pass git info as build args
    local git_args="--build-arg GIT_COMMIT_ID=${GIT_COMMIT_ID} --build-arg GIT_BRANCH=${GIT_BRANCH} --build-arg GIT_DESCRIBE=${GIT_DESCRIBE} --build-arg GIT_DIRTY=${GIT_DIRTY}"

    if [ "$CONTAINER_CMD" = "docker" ]; then
        docker buildx build \
            $build_args \
            --platform linux/${arch} \
            --build-arg TARGET_ARCH=${arch} \
            $git_args \
            --tag aasdk-build:${arch} \
            --load \
            .
        local container_id=$(docker create aasdk-build:${arch})
        docker cp ${container_id}:/output/. ./build-output/
        docker rm ${container_id} > /dev/null
    else
        podman build \
            $build_args \
            --arch ${arch} \
            --build-arg TARGET_ARCH=${arch} \
            $git_args \
            --tag aasdk-build:${arch} \
            .
        local container_id=$(podman create aasdk-build:${arch})
        podman cp ${container_id}:/output/. ./build-output/
        podman rm ${container_id} > /dev/null
    fi

    print_success "Build completed for ${arch}"
}

# Clean output directory if requested
if [ "$CLEAN" = true ]; then
    print_step "Cleaning build output directory..."
    rm -rf build-output
    mkdir -p build-output
fi

case $ARCH in
    amd64|arm64|armhf)
        platform_arch=$ARCH
        if [ "$ARCH" = "armhf" ]; then
            platform_arch="arm/v7"
        fi
        build_architecture $platform_arch "$NO_CACHE"
        ;;
    all)
        print_step "Building for all architectures..."
        build_architecture "amd64" "$NO_CACHE"
        build_architecture "arm64" "$NO_CACHE"
        build_architecture "arm/v7" "$NO_CACHE"
        print_success "All architectures built successfully"
        ;;
    *)
        print_error "Unknown architecture: $ARCH"
        show_usage
        exit 1
        ;;
esac

print_step "Build results:"
if [ -d "build-output" ]; then
    ls -la build-output/
    echo
    print_success "Packages are available in build-output/ directory"
else
    print_error "No build output directory found"
    exit 1
fi

print_success "Build process completed successfully!"