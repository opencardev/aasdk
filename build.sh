#!/bin/bash

# AASDK Build Script - Dependency-First Approach
# 1. Install system dependencies
# 2. Build and package custom dependencies (protobuf)
# 3. Build AASDK using installed dependencies
# 4. Package AASDK
#
# Usage:
#   ./build.sh [BUILD_TYPE] [OPTIONS]
#
# BUILD_TYPE:
#   debug      - Debug build with optimizations disabled
#   release    - Release build with optimizations enabled
#
# OPTIONS:
#   clean      - Clean build directory before building
#   test       - Run tests after building
#   install    - Install after building
#   package    - Create packages after building
#   deps-only  - Only build and package dependencies
#
# Environment Variables:
#   TARGET_ARCH   - Target architecture (amd64, arm64, armhf, i386)
#   JOBS          - Number of parallel build jobs (default: nproc-1)
#   CMAKE_ARGS    - Additional CMake arguments
#   CROSS_COMPILE - Enable cross-compilation (true/false, default: true)
#   DRY_RUN       - If set to true/1, skip any system installation steps

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Default values
if [ -z "$1" ]; then
    # Use environment variable if set, otherwise try to detect from git
    if [ -n "$GIT_BRANCH" ] && [ "$GIT_BRANCH" != "unknown" ]; then
        CURRENT_BRANCH="$GIT_BRANCH"
    else
        CURRENT_BRANCH=$(git rev-parse --abbrev-ref HEAD 2>/dev/null || echo "unknown")
    fi
    
    if [ "$CURRENT_BRANCH" = "main" ] || [ "$CURRENT_BRANCH" = "master" ] || [ "$CURRENT_BRANCH" = "development" ]; then
        BUILD_TYPE="release"
    else
        BUILD_TYPE="debug"
    fi
else
    BUILD_TYPE="$1"
fi
TARGET_ARCH=${TARGET_ARCH:-amd64}
NPROC=$(nproc 2>/dev/null || echo 1)
if [ "$NPROC" -gt 1 ]; then
    JOBS_DEFAULT=$((NPROC-1))
else
    JOBS_DEFAULT=1
fi
JOBS=${JOBS:-$JOBS_DEFAULT}
CMAKE_ARGS=${CMAKE_ARGS:-}
CROSS_COMPILE=${CROSS_COMPILE:-true}
DRY_RUN=${DRY_RUN:-false}

# Parse command line arguments
CLEAN=false
RUN_TESTS=false
INSTALL=false
CREATE_PACKAGES=false
DEPS_ONLY=false

for arg in "$@"; do
    case $arg in
        debug|release)
            BUILD_TYPE=$arg
            ;;
        clean)
            CLEAN=true
            ;;
        test)
            RUN_TESTS=true
            ;;
        install)
            INSTALL=true
            ;;
        package)
            CREATE_PACKAGES=true
            ;;
        deps-only)
            DEPS_ONLY=true
            ;;
        dryrun)
            DRY_RUN=true
            ;;
        *)
            # Unknown option
            ;;
    esac
done

# Functions
print_header() {
    echo -e "${BLUE}================================================${NC}"
    echo -e "${BLUE}  AASDK Build Script (Dependency-First)${NC}"
    echo -e "${BLUE}================================================${NC}"
    echo -e "Build Type:     ${GREEN}${BUILD_TYPE}${NC}"
    echo -e "Architecture:   ${GREEN}${TARGET_ARCH}${NC}"
    echo -e "Parallel Jobs:  ${GREEN}${JOBS}${NC}"
    echo -e "Deps Only:      ${GREEN}${DEPS_ONLY}${NC}"
    echo -e "Clean Build:    ${GREEN}${CLEAN}${NC}"
    echo -e "Run Tests:      ${GREEN}${RUN_TESTS}${NC}"
    echo -e "Install:        ${GREEN}${INSTALL}${NC}"
    echo -e "Create Packages: ${GREEN}${CREATE_PACKAGES}${NC}"
    echo -e "Dry Run:        ${GREEN}${DRY_RUN}${NC}"
    echo -e "${BLUE}================================================${NC}"
}

print_step() {
    echo -e "${BLUE}[STEP]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

# Install system dependencies
install_system_deps() {
    # Skip system dependency installation in Docker containers
    # The Dockerfile already installs all required dependencies
    if [ -f "/.dockerenv" ] || [ -n "$GIT_COMMIT_ID" ] || [ "$EUID" -eq 0 ]; then
        print_step "Running in Docker container - skipping system dependency installation"
        return 0
    fi

    if [ "$DRY_RUN" = true ]; then
        print_step "DRY RUN: Would install system dependencies"
        return 0
    fi

    print_step "Installing system dependencies..."

    # Update package list
    apt-get update

    # Install build tools and basic dependencies
    apt-get install -y \
        build-essential \
        cmake \
        pkg-config \
        git \
        wget \
        curl \
        libboost-system-dev \
        libboost-log-dev \
        libboost-thread-dev \
        libboost-chrono-dev \
        libboost-date-time-dev \
        libboost-atomic-dev \
        libboost-filesystem-dev \
        libusb-1.0-0-dev \
        libssl-dev \
        libboost-test-dev \
        dpkg-dev \
        debhelper

    print_success "System dependencies installed"
}

# Build and install protobuf dependency
build_protobuf_dependency() {
    print_step "Building protobuf dependency..."

    if [ "$CLEAN" = true ] && [ -d "protobuf/build" ]; then
        rm -rf protobuf/build
    fi

    mkdir -p protobuf/build
    cd protobuf/build

    # Configure protobuf as standalone
    cmake -DCMAKE_BUILD_TYPE=Release \
          -DTARGET_ARCH=$TARGET_ARCH \
          -DCMAKE_INSTALL_PREFIX="/usr/local" \
          $CMAKE_ARGS \
          ..

    # Build and install
    make -j$JOBS

    if [ "$DRY_RUN" = false ]; then
        make install
    fi

    # Create DEB package for protobuf
    if [ "$CREATE_PACKAGES" = true ]; then
        print_step "Creating protobuf DEB package..."
        cpack
    fi

    cd ../..

    print_success "Protobuf dependency built and installed"
}

# Build AASDK
build_aasdk() {
    print_step "Building AASDK..."

    local build_dir="build-${BUILD_TYPE}"
    if [ "$TARGET_ARCH" != "amd64" ]; then
        build_dir="build-${BUILD_TYPE}-${TARGET_ARCH}"
    fi

    if [ "$CLEAN" = true ] && [ -d "$build_dir" ]; then
        rm -rf "$build_dir"
    fi

    mkdir -p "$build_dir"
    cd "$build_dir"

    # Convert build type to proper case
    local cmake_build_type
    case $BUILD_TYPE in
        debug)
            cmake_build_type="Debug"
            ;;
        release)
            cmake_build_type="Release"
            ;;
        *)
            cmake_build_type="Release"
            ;;
    esac

    # Configure AASDK
    cmake -DCMAKE_BUILD_TYPE=$cmake_build_type \
          -DTARGET_ARCH=$TARGET_ARCH \
          -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
          -DBUILD_TESTING=ON \
          $CMAKE_ARGS \
          ..

    # Build
    make -j$JOBS

    cd ..

    export BUILD_DIR="$build_dir"
    print_success "AASDK built successfully"
}

# Run tests
run_tests() {
    if [ "$RUN_TESTS" = true ] && [ -n "$BUILD_DIR" ]; then
        print_step "Running tests..."
        cd "$BUILD_DIR"
        ctest --output-on-failure
        cd ..
        print_success "Tests completed"
    fi
}

# Install AASDK
install_aasdk() {
    if [ "$INSTALL" = true ] && [ -n "$BUILD_DIR" ]; then
        print_step "Installing AASDK..."
        cd "$BUILD_DIR"
        if [ "$DRY_RUN" = false ]; then
            make install
        else
            print_step "DRY RUN: Would install AASDK"
        fi
        cd ..
        print_success "AASDK installed"
    fi
}

# Create AASDK packages
create_aasdk_packages() {
    if [ "$CREATE_PACKAGES" = true ] && [ -n "$BUILD_DIR" ]; then
        print_step "Creating AASDK packages..."
        cd "$BUILD_DIR"
        cpack --config CPackConfig.cmake
        cd ..

        # Move packages to top-level packages directory
        mkdir -p packages
        mv "$BUILD_DIR"/*.deb packages/ 2>/dev/null || true
        mv "$BUILD_DIR"/*.tar.* packages/ 2>/dev/null || true
        mv protobuf/build/*.deb packages/ 2>/dev/null || true

        print_success "Packages created in packages/ directory"
    fi
}

# Show build summary
show_build_summary() {
    echo
    echo -e "${BLUE}================================================${NC}"
    echo -e "${GREEN}Build completed successfully!${NC}"
    echo -e "${BLUE}================================================${NC}"

    if [ "$CREATE_PACKAGES" = true ]; then
        echo -e "${GREEN}✅ Packages created${NC}"
        if [ -d "packages" ]; then
            echo -e "${BLUE}Created packages:${NC}"
            ls -la packages/
        fi
    fi

    echo
    echo -e "${YELLOW}Next steps:${NC}"
    if [ -n "$BUILD_DIR" ]; then
        echo -e "  • To run tests: cd ${BUILD_DIR} && ctest"
        echo -e "  • To install: cd ${BUILD_DIR} && sudo make install"
        echo -e "  • To create packages: cd ${BUILD_DIR} && cpack"
    fi
    echo -e "  • For troubleshooting: see TROUBLESHOOTING.md"
    echo -e "${BLUE}================================================${NC}"
}

# Show usage
show_usage() {
    echo "AASDK Build Script (Dependency-First Approach)"
    echo
    echo "Usage: $0 [BUILD_TYPE] [OPTIONS]"
    echo
    echo "BUILD_TYPE:"
    echo "  debug       Debug build with optimizations disabled (default)"
    echo "  release     Release build with optimizations enabled"
    echo
    echo "OPTIONS:"
    echo "  clean       Clean build directory before building"
    echo "  test        Run tests after building"
    echo "  install     Install after building"
    echo "  package     Create packages after building"
    echo "  deps-only   Only build and package dependencies"
    echo
    echo "Environment Variables:"
    echo "  TARGET_ARCH    Target architecture (amd64, arm64, armhf, i386)"
    echo "  JOBS           Number of parallel build jobs (default: nproc-1)"
    echo "  CMAKE_ARGS     Additional CMake arguments"
    echo "  CROSS_COMPILE  Enable cross-compilation (true/false, default: true)"
    echo
    echo "Examples:"
    echo "  $0 debug                    # Debug build"
    echo "  $0 release clean           # Clean release build"
    echo "  $0 debug test              # Debug build with tests"
    echo "  TARGET_ARCH=arm64 $0 release  # Cross-compile for ARM64"
    echo "  $0 deps-only package       # Only build dependencies and package them"
    echo
    echo "For complete documentation, see BUILD.md"
}

# Main execution
main() {
    # Check for help
    if [ "$1" = "-h" ] || [ "$1" = "--help" ] || [ "$1" = "help" ]; then
        show_usage
        exit 0
    fi

    # Validate build type
    if [ "$BUILD_TYPE" != "debug" ] && [ "$BUILD_TYPE" != "release" ]; then
        print_error "Invalid build type: $BUILD_TYPE"
        echo "Valid build types: debug, release"
        exit 1
    fi

    print_header

    # Phase 1: Install system dependencies
    install_system_deps

    # Phase 2: Build and install custom dependencies
    build_protobuf_dependency

    # Stop here if only building dependencies
    if [ "$DEPS_ONLY" = true ]; then
        print_success "Dependencies built successfully"
        if [ "$CREATE_PACKAGES" = true ] && [ -d "packages" ]; then
            echo -e "${BLUE}Dependency packages:${NC}"
            ls -la packages/
        fi
        exit 0
    fi

    # Phase 3: Build AASDK
    build_aasdk

    # Phase 4: Test, install, package
    run_tests
    install_aasdk
    create_aasdk_packages

    show_build_summary
}

# Execute main function
main "$@"
