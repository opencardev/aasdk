# AASDK Build Guide

This comprehensive guide covers all aspects of building AASDK, from quick setup to advanced cross-compilation scenarios.

## Table of Contents

1. [Quick Start](#quick-start)
2. [Prerequisites](#prerequisites)
3. [Build Methods](#build-methods)
4. [Configuration Options](#configuration-options)
5. [Cross-Compilation](#cross-compilation)
6. [Advanced Build Scenarios](#advanced-build-scenarios)
7. [Build Optimization](#build-optimization)
8. [Packaging](#packaging)

---

## Quick Start

### DevContainer Build (Recommended)

**Fastest way to get started:**
```bash
# 1. Open VS Code in project directory
# 2. Ctrl+Shift+P → "Dev Containers: Reopen in Container"
# 3. Build with single command
./build.sh debug
```

### Native Build

**Ubuntu/Debian Quick Setup:**
```bash
# Install dependencies
sudo apt update && sudo apt install -y \
    build-essential cmake git pkg-config \
    libboost-all-dev libusb-1.0-0-dev libssl-dev \
    libprotobuf-dev protobuf-compiler

# Build
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
```

---

## Prerequisites

### System Requirements

| Component | Minimum | Recommended |
|-----------|---------|-------------|
| **OS** | Ubuntu 18.04+ | Ubuntu 22.04+ |
| **RAM** | 4GB | 8GB+ |
| **Disk** | 2GB free | 10GB+ free |
| **CPU** | 2 cores | 4+ cores |

### Required Dependencies

#### Core Build Tools
```bash
# Ubuntu/Debian
sudo apt install build-essential cmake git pkg-config

# CentOS/RHEL/Fedora
sudo dnf groupinstall "Development Tools"
sudo dnf install cmake git pkgconfig
```

#### Libraries

**Boost Libraries:**
```bash
# Ubuntu/Debian
sudo apt install libboost-all-dev

# CentOS/RHEL
sudo dnf install boost-devel

# Alpine Linux
sudo apk add boost-dev

# macOS
brew install boost
```

**USB Support:**
```bash
# Ubuntu/Debian
sudo apt install libusb-1.0-0-dev

# CentOS/RHEL
sudo dnf install libusb1-devel

# macOS
brew install libusb
```

**SSL/TLS:**
```bash
# Ubuntu/Debian
sudo apt install libssl-dev

# CentOS/RHEL
sudo dnf install openssl-devel

# macOS (usually pre-installed)
brew install openssl
```

**Protocol Buffers:**
```bash
# Ubuntu/Debian
sudo apt install libprotobuf-dev protobuf-compiler

# CentOS/RHEL
sudo dnf install protobuf-devel protobuf-compiler

# macOS
brew install protobuf
```

#### Testing Dependencies (Optional)
```bash
# Google Test framework
sudo apt install googletest libgtest-dev

# Code coverage tools
sudo apt install gcov lcov

# Memory debugging
sudo apt install valgrind
```

### Version Requirements

| Dependency | Minimum Version | Recommended |
|------------|----------------|-------------|
| **CMake** | 3.14+ | 3.20+ |
| **GCC** | 7.0+ | 9.0+ |
| **Boost** | 1.71+ | 1.81+ |
| **Protobuf** | 3.21+ | 3.21+ |
| **OpenSSL** | 1.1+ | 3.0+ |

---

## Build Methods

### Method 1: Build Scripts (Recommended)

The project includes convenience scripts for common build scenarios:

**Linux/macOS/DevContainer:**
```bash
./build.sh debug     # Debug build with optimizations disabled
./build.sh release   # Release build with optimizations enabled
./build.sh debug clean    # Clean debug build
./build.sh release clean  # Clean release build
./build.sh debug test     # Debug build with tests
./build.sh release package # Release build with package creation
```

**Windows:**
```powershell
# PowerShell (recommended)
.\build.ps1 debug
.\build.ps1 release clean
.\build.ps1 -BuildType debug -TargetArch amd64 clean test

# Batch file (basic wrapper)
.\build.bat debug
.\build.bat release clean
```

**Script Features:**
- Automatic dependency checking
- Cross-platform support (Linux, macOS, Windows)
- Parallel compilation
- Build validation
- Error reporting
- Cross-compilation support
- Package creation
- Test execution

### Method 2: VS Code Tasks

Use integrated VS Code tasks for streamlined development:

**Available Tasks:**
- `DevContainer: Build Debug (Quick)`
- `DevContainer: Build Release (Quick)`
- `DevContainer: Clean & Build`
- `DevContainer: Create Packages`

**Usage:**
```
Ctrl+Shift+P → "Tasks: Run Task" → Select desired task
```

### Method 3: Manual CMake

**Debug Build:**
```bash
mkdir -p build-debug
cd build-debug
cmake -DCMAKE_BUILD_TYPE=Debug \
      -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
      ..
make -j$(nproc)
```

**Release Build:**
```bash
mkdir -p build-release
cd build-release
cmake -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
      ..
make -j$(nproc)
```

**Install Build:**
```bash
cd build-release
make install
# Or with custom prefix:
cmake -DCMAKE_INSTALL_PREFIX=/opt/aasdk ..
make install
```

### Method 4: Package Managers

**Using vcpkg:**
```bash
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg && ./bootstrap-vcpkg.sh
./vcpkg install boost-system boost-log libusb openssl protobuf

# Build with vcpkg
cmake -DCMAKE_TOOLCHAIN_FILE=/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake ..
```

**Using Conan:**
```bash
# Install Conan
pip install conan

# Create conanfile.txt
cat > conanfile.txt << EOF
[requires]
boost/1.81.0
libusb/1.0.26
openssl/3.0.9
protobuf/3.21.12

[generators]
cmake
EOF

# Install dependencies and build
mkdir build && cd build
conan install ..
cmake ..
make -j$(nproc)
```

---

## Configuration Options

### Build Types

| Build Type | Optimization | Debug Info | Use Case |
|------------|-------------|------------|----------|
| **Debug** | -O0 | Full | Development, debugging |
| **Release** | -O3 | Minimal | Production deployment |
| **RelWithDebInfo** | -O2 | Full | Performance analysis |
| **MinSizeRel** | -Os | Minimal | Embedded systems |

### CMake Options

**Core Options:**
```bash
# Build configuration
-DCMAKE_BUILD_TYPE=Release          # Release build
-DTARGET_ARCH=amd64                 # Target architecture
-DBUILD_TESTING=ON                  # Enable unit tests
-DBUILD_SHARED_LIBS=ON              # Build shared library

# Installation paths
-DCMAKE_INSTALL_PREFIX=/usr/local   # Install prefix
-DCMAKE_INSTALL_LIBDIR=lib          # Library directory
-DCMAKE_INSTALL_INCLUDEDIR=include  # Header directory

# Advanced options
-DENABLE_COVERAGE=ON                # Code coverage
-DUSE_BUNDLED_PROTOBUF=OFF          # Use system protobuf
-DENABLE_STATIC_ANALYSIS=ON         # Static analysis tools
```

**Compiler Flags:**
```bash
# Debug flags
-DCMAKE_CXX_FLAGS_DEBUG="-g3 -O0 -fno-omit-frame-pointer"

# Release flags
-DCMAKE_CXX_FLAGS_RELEASE="-O3 -DNDEBUG -march=native"

# Security hardening
-DCMAKE_CXX_FLAGS="-fstack-protector-strong -D_FORTIFY_SOURCE=2"

# Sanitizers
-DCMAKE_CXX_FLAGS="-fsanitize=address,undefined"
```

### Environment Variables

```bash
# Compilation
export CC=gcc-9
export CXX=g++-9
export CMAKE_BUILD_PARALLEL_LEVEL=8

# Library paths
export PKG_CONFIG_PATH="/usr/local/lib/pkgconfig:$PKG_CONFIG_PATH"
export LD_LIBRARY_PATH="/usr/local/lib:$LD_LIBRARY_PATH"

# Boost configuration
export BOOST_ROOT=/usr/local
export Boost_NO_SYSTEM_PATHS=ON
```

---

## Cross-Compilation

### Supported Architectures

| Architecture | CMake Target | Compiler | Use Case |
|-------------|-------------|----------|----------|
| **x64** | amd64 | gcc/g++ | Desktop, servers |
| **ARM64** | arm64 | aarch64-linux-gnu-gcc | Raspberry Pi 4, modern ARM |
| **ARMHF** | armhf | arm-linux-gnueabihf-gcc | Raspberry Pi 3, legacy ARM |
| **i386** | i386 | gcc -m32 | Legacy x86 systems |

### Cross-Compilation Setup

**Install Cross-Compilers:**
```bash
# ARM64 toolchain
sudo apt install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu

# ARMHF toolchain
sudo apt install gcc-arm-linux-gnueabihf g++-arm-linux-gnueabihf

# Multi-arch libraries
sudo dpkg --add-architecture arm64
sudo dpkg --add-architecture armhf
sudo apt update
sudo apt install libboost-all-dev:arm64 libusb-1.0-0-dev:arm64
sudo apt install libboost-all-dev:armhf libusb-1.0-0-dev:armhf
```

**ARM64 Cross-Compilation:**
```bash
mkdir build-arm64
cd build-arm64
cmake -DCMAKE_BUILD_TYPE=Release \
      -DTARGET_ARCH=arm64 \
      -DCMAKE_C_COMPILER=aarch64-linux-gnu-gcc \
      -DCMAKE_CXX_COMPILER=aarch64-linux-gnu-g++ \
      -DCMAKE_FIND_ROOT_PATH=/usr/aarch64-linux-gnu \
      -DCMAKE_FIND_ROOT_PATH_MODE_PROGRAM=NEVER \
      -DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=ONLY \
      -DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=ONLY \
      ..
make -j$(nproc)
```

**ARMHF Cross-Compilation:**
```bash
mkdir build-armhf
cd build-armhf
cmake -DCMAKE_BUILD_TYPE=Release \
      -DTARGET_ARCH=armhf \
      -DCMAKE_C_COMPILER=arm-linux-gnueabihf-gcc \
      -DCMAKE_CXX_COMPILER=arm-linux-gnueabihf-g++ \
      -DCMAKE_FIND_ROOT_PATH=/usr/arm-linux-gnueabihf \
      -DCMAKE_FIND_ROOT_PATH_MODE_PROGRAM=NEVER \
      -DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=ONLY \
      -DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=ONLY \
      ..
make -j$(nproc)
```

### Custom Toolchain Files

**Create `cmake/arm64-toolchain.cmake`:**
```cmake
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

set(CMAKE_C_COMPILER aarch64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)

set(CMAKE_FIND_ROOT_PATH /usr/aarch64-linux-gnu)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=armv8-a")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=armv8-a")
```

**Use Toolchain File:**
```bash
cmake -DCMAKE_TOOLCHAIN_FILE=cmake/arm64-toolchain.cmake ..
```

---

## Advanced Build Scenarios

### Minimal Embedded Build

For resource-constrained embedded systems:

```bash
cmake -DCMAKE_BUILD_TYPE=MinSizeRel \
      -DBUILD_TESTING=OFF \
      -DBUILD_SHARED_LIBS=OFF \
      -DENABLE_LOGGING=OFF \
      -DCMAKE_CXX_FLAGS="-Os -ffunction-sections -fdata-sections" \
      -DCMAKE_EXE_LINKER_FLAGS="-Wl,--gc-sections" \
      ..
```

### Development Build with All Tools

For comprehensive development environment:

```bash
cmake -DCMAKE_BUILD_TYPE=Debug \
      -DBUILD_TESTING=ON \
      -DENABLE_COVERAGE=ON \
      -DENABLE_STATIC_ANALYSIS=ON \
      -DCMAKE_CXX_FLAGS="-g3 -O0 -fsanitize=address,undefined -fno-omit-frame-pointer" \
      -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
      ..
```

### Static Analysis Build

Enable static analysis tools:

```bash
# Install analysis tools
sudo apt install cppcheck clang-tidy

# Configure build with analysis
cmake -DCMAKE_BUILD_TYPE=Debug \
      -DENABLE_CPPCHECK=ON \
      -DENABLE_CLANG_TIDY=ON \
      -DENABLE_INCLUDE_WHAT_YOU_USE=ON \
      ..
```

### Continuous Integration Build

Optimized for CI environments:

```bash
cmake -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_TESTING=ON \
      -DENABLE_COVERAGE=ON \
      -DCMAKE_BUILD_PARALLEL_LEVEL=${CI_PARALLEL_JOBS:-4} \
      -DCMAKE_VERBOSE_MAKEFILE=ON \
      ..
make -j${CI_PARALLEL_JOBS:-4}
ctest --parallel ${CI_PARALLEL_JOBS:-4} --output-on-failure
```

---

## Build Optimization

### Compilation Speed

**Parallel Compilation:**
```bash
# Use all CPU cores
make -j$(nproc)

# Limit parallel jobs to avoid memory issues
make -j$(($(nproc) / 2))

# Use Ninja for faster builds
cmake -GNinja ..
ninja -j$(nproc)
```

**Compiler Cache:**
```bash
# Install ccache
sudo apt install ccache

# Configure CMake to use ccache
cmake -DCMAKE_C_COMPILER_LAUNCHER=ccache \
      -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
      ..

# Monitor cache effectiveness
ccache -s
```

**Precompiled Headers:**
```cmake
# In CMakeLists.txt
target_precompile_headers(aasdk PRIVATE
    <boost/system/error_code.hpp>
    <boost/log/trivial.hpp>
    <memory>
    <vector>
    <string>
)
```

### Binary Size Optimization

**Release Size Optimization:**
```bash
cmake -DCMAKE_BUILD_TYPE=MinSizeRel \
      -DCMAKE_CXX_FLAGS="-Os -flto" \
      -DCMAKE_EXE_LINKER_FLAGS="-Wl,--strip-all,--gc-sections" \
      ..
```

**Strip Debug Symbols:**
```bash
# After build
strip build-release/lib/libaasdk.so
strip build-release/bin/*

# Or during build
cmake -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_CXX_FLAGS_RELEASE="-O3 -DNDEBUG -s" \
      ..
```

### Runtime Performance

**Profile-Guided Optimization:**
```bash
# Step 1: Build with profiling
cmake -DCMAKE_CXX_FLAGS="-fprofile-generate" ..
make -j$(nproc)

# Step 2: Run representative workload
./your_app_with_typical_usage

# Step 3: Rebuild with profile data
make clean
cmake -DCMAKE_CXX_FLAGS="-fprofile-use" ..
make -j$(nproc)
```

**Link-Time Optimization:**
```bash
cmake -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_CXX_FLAGS="-flto" \
      -DCMAKE_EXE_LINKER_FLAGS="-flto" \
      ..
```

---

## Packaging

### DEB Package Creation

**Standard Package Build:**
```bash
cd build-release
cpack --config CPackConfig.cmake
```

**Custom Package Configuration:**
```cmake
# In CMakeLists.txt
set(CPACK_GENERATOR "DEB")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Your Name <email@example.com>")
set(CPACK_DEBIAN_PACKAGE_DESCRIPTION "AndroidAuto SDK Library")
set(CPACK_DEBIAN_PACKAGE_SECTION "libs")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libboost-system1.81.0, libusb-1.0-0, libssl3")
```

### Multiple Package Types

**Create All Package Types:**
```bash
# DEB packages
cpack -G DEB

# RPM packages
cpack -G RPM

# TAR.GZ archives
cpack -G TGZ

# Source packages
cpack --config CPackSourceConfig.cmake
```

### Package Testing

**Test Package Installation:**
```bash
# Install locally built package
sudo dpkg -i libaasdk*.deb

# Verify installation
pkg-config --modversion aasdk
ldconfig -p | grep aasdk

# Test package removal
sudo dpkg -r libaasdk
```

**Package Dependency Testing:**
```bash
# Check package dependencies
dpkg-deb -I libaasdk*.deb
apt-cache show libaasdk

# Test on clean system
docker run -it ubuntu:22.04
# Inside container: install and test package
```

---

## Build Validation

### Post-Build Checks

**Automated Validation Script:**
```bash
#!/bin/bash
# validate_build.sh

echo "🔍 Validating AASDK Build..."

# Check library exists
if [ -f "lib/libaasdk.so" ]; then
    echo "✅ Library built successfully"
else
    echo "❌ Library missing"
    exit 1
fi

# Check symbols
if nm lib/libaasdk.so | grep -q "aasdk"; then
    echo "✅ Library symbols present"
else
    echo "❌ Missing expected symbols"
    exit 1
fi

# Check dependencies
ldd lib/libaasdk.so
if [ $? -eq 0 ]; then
    echo "✅ Dependencies resolved"
else
    echo "❌ Dependency issues"
    exit 1
fi

# Run unit tests
if ctest --output-on-failure; then
    echo "✅ Unit tests passed"
else
    echo "❌ Unit test failures"
    exit 1
fi

echo "🎉 Build validation completed successfully!"
```

### Quality Checks

**Code Quality Validation:**
```bash
# Static analysis
cppcheck --enable=all --error-exitcode=1 src/

# Memory leak check
valgrind --error-exitcode=1 --leak-check=full ./test_program

# Thread safety check
clang++ -fsanitize=thread -g src/*.cpp

# Code coverage
gcov src/*.cpp
lcov --capture --directory . --output-file coverage.info
```

---

## Troubleshooting Build Issues

### Common Problems

**Problem: CMake can't find dependencies**
```bash
# Solution: Set explicit paths
cmake -DBoost_ROOT=/usr/local \
      -DPROTOBUF_ROOT=/usr/local \
      ..
```

**Problem: Compilation memory issues**
```bash
# Solution: Reduce parallel jobs
make -j2  # Instead of -j$(nproc)

# Or increase swap space
sudo fallocate -l 4G /swapfile
sudo chmod 600 /swapfile
sudo mkswap /swapfile
sudo swapon /swapfile
```

**Problem: Cross-compilation failures**
```bash
# Solution: Verify toolchain installation
arm-linux-gnueabihf-gcc --version

# Check library paths
export CMAKE_FIND_ROOT_PATH=/usr/arm-linux-gnueabihf
```

For comprehensive troubleshooting, see [TROUBLESHOOTING.md](TROUBLESHOOTING.md).

---

## Build Scripts Reference

### Project Build Scripts

**`build.sh` (Linux/macOS/DevContainer):**
```bash
# Basic usage
./build.sh [BUILD_TYPE] [OPTIONS]

# BUILD_TYPE: debug, release
# OPTIONS: clean, test, install, package

# Examples
./build.sh debug          # Debug build
./build.sh release        # Release build
./build.sh debug clean    # Clean debug build
./build.sh release test   # Release build with tests
./build.sh debug install  # Debug build with installation
./build.sh release package # Release build with package creation

# Environment variables
TARGET_ARCH=arm64 ./build.sh release   # Cross-compile for ARM64
JOBS=4 ./build.sh debug                # Limit parallel jobs
CMAKE_ARGS="-DENABLE_COVERAGE=ON" ./build.sh debug # Additional CMake args
```

**`build.ps1` (Windows PowerShell):**
```powershell
# Basic usage
.\build.ps1 [BuildType] [Options]

# Examples
.\build.ps1 debug
.\build.ps1 release clean
.\build.ps1 -BuildType debug -TargetArch amd64 test
.\build.ps1 release -Jobs 4 package

# Parameters
-BuildType      # debug, release
-TargetArch     # amd64, x86
-Jobs           # Number of parallel jobs
-CMakeArgs      # Additional CMake arguments
```

**`build.bat` (Windows Batch):**
```batch
REM Simple wrapper that detects and uses available Unix environment
.\build.bat debug
.\build.bat release clean
```

**Custom Build Script Example:**
```bash
#!/bin/bash
# custom_build.sh

set -e  # Exit on error

BUILD_TYPE=${1:-Release}
TARGET_ARCH=${2:-amd64}
CLEAN=${3:-false}

echo "Building AASDK: $BUILD_TYPE for $TARGET_ARCH"

if [ "$CLEAN" = "clean" ]; then
    rm -rf build-*
fi

mkdir -p build-$TARGET_ARCH
cd build-$TARGET_ARCH

cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
      -DTARGET_ARCH=$TARGET_ARCH \
      ..

make -j$(nproc)

echo "Build completed: build-$TARGET_ARCH/"
```

---

*This build guide provides comprehensive coverage of all AASDK build scenarios. For specific issues, consult the troubleshooting guide or community support.*
