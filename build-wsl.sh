#!/bin/bash
# Simple build script for WSL environment
# Usage: ./build-wsl.sh [debug|release] [clean]

set -e

BUILD_TYPE="${1:-release}"
DO_CLEAN="${2}"

if [[ "$BUILD_TYPE" == "debug" ]]; then
    BUILD_DIR="build-debug"
    CMAKE_BUILD_TYPE="Debug"
else
    BUILD_DIR="build-release"
    CMAKE_BUILD_TYPE="Release"
fi

echo "========================================"
echo "Building AASDK ($CMAKE_BUILD_TYPE)"
echo "========================================"

# Clean if requested
if [[ "$DO_CLEAN" == "clean" ]]; then
    echo "Cleaning build directories..."
    rm -rf build-debug build-release protobuf/build
fi

# Build protobuf first
echo "Building protobuf library..."
cd protobuf
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=$CMAKE_BUILD_TYPE ..
# Use -j2 to avoid memory issues in WSL
make -j2
cd ../..

# Build main library
echo "Building main library..."
mkdir -p $BUILD_DIR
cd $BUILD_DIR
cmake -DCMAKE_BUILD_TYPE=$CMAKE_BUILD_TYPE ..
make -j2
cd ..

echo "========================================"
echo "Build completed successfully!"
echo "========================================"
