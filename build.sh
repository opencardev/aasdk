#!/bin/bash

# AASDK Build Script
# Provides convenient building for different configurations and architectures
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
BUILD_TYPE=${1:-debug}
TARGET_ARCH=${TARGET_ARCH:-amd64}
# Use one fewer core by default to reduce memory pressure on small devices
NPROC=$(nproc 2>/dev/null || echo 1)
if [ "$NPROC" -gt 1 ]; then
    JOBS_DEFAULT=$((NPROC-1))
else
    JOBS_DEFAULT=1
fi
JOBS=${JOBS:-$JOBS_DEFAULT}
CMAKE_ARGS=${CMAKE_ARGS:-}
CROSS_COMPILE=${CROSS_COMPILE:-true}
# Dry run mode (skip system install)
DRY_RUN=${DRY_RUN:-false}

# Parse command line arguments
CLEAN=false
RUN_TESTS=false
INSTALL=false
CREATE_PACKAGES=false

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
    echo -e "${BLUE}  AASDK Build Script${NC}"
    echo -e "${BLUE}================================================${NC}"
    echo -e "Build Type:     ${GREEN}${BUILD_TYPE}${NC}"
    echo -e "Architecture:   ${GREEN}${TARGET_ARCH}${NC}"
    echo -e "Parallel Jobs:  ${GREEN}${JOBS}${NC}"
    echo -e "Clean Build:    ${GREEN}${CLEAN}${NC}"
    echo -e "Run Tests:      ${GREEN}${RUN_TESTS}${NC}"
    echo -e "Install:        ${GREEN}${INSTALL}${NC}"
    echo -e "Create Packages: ${GREEN}${CREATE_PACKAGES}${NC}"
    echo -e "Dry Run:        ${GREEN}${DRY_RUN}${NC}"
    echo -e "${YELLOW}Git details:${NC}"
    echo "  GIT_COMMIT_ID: $GIT_COMMIT_ID"
    echo "  GIT_BRANCH:    $GIT_BRANCH"
    echo "  GIT_DESCRIBE:  $GIT_DESCRIBE"
    echo "  GIT_TIMESTAMP: $GIT_COMMIT_TIMESTAMP"
    echo "  GIT_DIRTY:     $GIT_DIRTY"
    echo -e "${BLUE}================================================${NC}"
    echo
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

check_dependencies() {
    print_step "Checking dependencies..."
    
    # Check for required tools
    local missing_deps=()
    
    if ! command -v cmake &> /dev/null; then
        missing_deps+=("cmake")
    fi
    
    if ! command -v make &> /dev/null; then
        missing_deps+=("build-essential")
    fi
    
    if ! command -v pkg-config &> /dev/null; then
        missing_deps+=("pkg-config")
    fi
    
    # Check for required libraries
    if ! pkg-config --exists protobuf; then
        missing_deps+=("libprotobuf-dev protobuf-compiler")
    fi
    
    if ! ldconfig -p | grep -q libboost_system; then
        missing_deps+=("libboost-all-dev")
    fi
    
    if ! ldconfig -p | grep -q libusb-1.0; then
        missing_deps+=("libusb-1.0-0-dev")
    fi
    
    if ! ldconfig -p | grep -q libssl; then
        missing_deps+=("libssl-dev")
    fi
    
    if [ ${#missing_deps[@]} -ne 0 ]; then
        print_error "Missing dependencies detected:"
        printf '%s\n' "${missing_deps[@]}"
        echo
        echo -e "${YELLOW}To install missing dependencies on Ubuntu/Debian:${NC}"
        echo "sudo apt update && sudo apt install -y ${missing_deps[*]}"
        echo
        echo -e "${YELLOW}Or use the DevContainer for automatic dependency management.${NC}"
        exit 1
    fi
    
    print_success "All dependencies found"
}

setup_native_compilation() {
    print_step "Setting up native compilation for ${TARGET_ARCH}..."
    
    # Force native compilers
    export CC=/usr/bin/cc
    export CXX=/usr/bin/c++
    export CMAKE_C_COMPILER=/usr/bin/cc
    export CMAKE_CXX_COMPILER=/usr/bin/c++
    
    # Add compiler settings to CMAKE_ARGS
    CMAKE_ARGS="$CMAKE_ARGS -DCMAKE_C_COMPILER=/usr/bin/cc"
    CMAKE_ARGS="$CMAKE_ARGS -DCMAKE_CXX_COMPILER=/usr/bin/c++"
    
    # Architecture-specific library paths for dependency detection
    local multiarch_path
    case $TARGET_ARCH in
        amd64)
            multiarch_path="x86_64-linux-gnu"
            ;;
        arm64)  
            multiarch_path="aarch64-linux-gnu"
            ;;
        armhf)
            multiarch_path="arm-linux-gnueabihf"
            ;;
        i386)
            multiarch_path="i386-linux-gnu"
            ;;
        *)
            print_error "Unsupported architecture: $TARGET_ARCH"
            echo "Supported architectures: amd64, arm64, armhf, i386"
            exit 1
            ;;
    esac
    
    # Add dependency paths to CMAKE_ARGS for common libraries
    CMAKE_ARGS="$CMAKE_ARGS -DProtobuf_INCLUDE_DIR=/usr/include"
    CMAKE_ARGS="$CMAKE_ARGS -DProtobuf_LIBRARIES=/usr/lib/${multiarch_path}/libprotobuf.so"
    CMAKE_ARGS="$CMAKE_ARGS -DProtobuf_LIBRARY=/usr/lib/${multiarch_path}/libprotobuf.so"
    CMAKE_ARGS="$CMAKE_ARGS -DProtobuf_LITE_LIBRARY=/usr/lib/${multiarch_path}/libprotobuf-lite.so"
    CMAKE_ARGS="$CMAKE_ARGS -DProtobuf_PROTOC_EXECUTABLE=/usr/bin/protoc"
    CMAKE_ARGS="$CMAKE_ARGS -DLIBUSB_1_INCLUDE_DIRS=/usr/include/libusb-1.0"
    CMAKE_ARGS="$CMAKE_ARGS -DLIBUSB_1_LIBRARIES=/usr/lib/${multiarch_path}/libusb-1.0.so"
    CMAKE_ARGS="$CMAKE_ARGS -DOPENSSL_INCLUDE_DIR=/usr/include/openssl"
    CMAKE_ARGS="$CMAKE_ARGS -DOPENSSL_CRYPTO_LIBRARY=/usr/lib/${multiarch_path}/libcrypto.so"
    CMAKE_ARGS="$CMAKE_ARGS -DOPENSSL_SSL_LIBRARY=/usr/lib/${multiarch_path}/libssl.so"
    
    print_success "Native compilation configured for ${TARGET_ARCH}"
}

setup_cross_compilation() {
    if [ "$TARGET_ARCH" != "amd64" ] && [ "$CROSS_COMPILE" = "true" ]; then
        print_step "Setting up cross-compilation for ${TARGET_ARCH}..."
        
        case $TARGET_ARCH in
            arm64)
                export CMAKE_C_COMPILER=aarch64-linux-gnu-gcc
                export CMAKE_CXX_COMPILER=aarch64-linux-gnu-g++
                CMAKE_ARGS="$CMAKE_ARGS -DCMAKE_FIND_ROOT_PATH=/usr/aarch64-linux-gnu"
                CMAKE_ARGS="$CMAKE_ARGS -DCMAKE_FIND_ROOT_PATH_MODE_PROGRAM=NEVER"
                CMAKE_ARGS="$CMAKE_ARGS -DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=ONLY"
                CMAKE_ARGS="$CMAKE_ARGS -DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=ONLY"
                
                if ! command -v aarch64-linux-gnu-gcc &> /dev/null; then
                    print_error "ARM64 cross-compiler not found"
                    echo "Install with: sudo apt install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu"
                    exit 1
                fi
                ;;
            armhf)
                export CMAKE_C_COMPILER=arm-linux-gnueabihf-gcc
                export CMAKE_CXX_COMPILER=arm-linux-gnueabihf-g++
                CMAKE_ARGS="$CMAKE_ARGS -DCMAKE_FIND_ROOT_PATH=/usr/arm-linux-gnueabihf"
                CMAKE_ARGS="$CMAKE_ARGS -DCMAKE_FIND_ROOT_PATH_MODE_PROGRAM=NEVER"
                CMAKE_ARGS="$CMAKE_ARGS -DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=ONLY"
                CMAKE_ARGS="$CMAKE_ARGS -DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=ONLY"
                
                if ! command -v arm-linux-gnueabihf-gcc &> /dev/null; then
                    print_error "ARMHF cross-compiler not found"
                    echo "Install with: sudo apt install gcc-arm-linux-gnueabihf g++-arm-linux-gnueabihf"
                    exit 1
                fi
                ;;
            i386)
                CMAKE_ARGS="$CMAKE_ARGS -DCMAKE_C_FLAGS=-m32 -DCMAKE_CXX_FLAGS=-m32"
                ;;
            *)
                print_error "Unsupported architecture: $TARGET_ARCH"
                echo "Supported architectures: amd64, arm64, armhf, i386"
                exit 1
                ;;
        esac
        
        print_success "Cross-compilation configured for ${TARGET_ARCH}"
    elif [ "$CROSS_COMPILE" = "false" ]; then
        setup_native_compilation
    fi
}

build_protobuf() {
    if [ ! -d "protobuf/build" ] || [ "$CLEAN" = true ]; then
        print_step "Building AAP Protobuf dependency..."
        
        if [ "$CLEAN" = true ] && [ -d "protobuf/build" ]; then
            rm -rf protobuf/build
        fi
        
        mkdir -p protobuf/build
        cd protobuf/build
        # Stage installs under the project to avoid requiring root
        STAGING_DIR="$(pwd)/_staging"
        mkdir -p "$STAGING_DIR"
        
      cmake -DCMAKE_BUILD_TYPE=Release \
          -DTARGET_ARCH=$TARGET_ARCH \
          -DCMAKE_INSTALL_PREFIX="/usr/local" \
          $CMAKE_ARGS \
          ..
        
    make -j$JOBS
    # Install to a local staging dir so no sudo is required
    make install DESTDIR="$STAGING_DIR"
        
    # Ensure main build can discover the staged install if it uses find_package
    export CMAKE_PREFIX_PATH="$STAGING_DIR/usr/local:${CMAKE_PREFIX_PATH}"
        cd ../..
        
        print_success "AAP Protobuf built successfully"
    else
        print_step "AAP Protobuf already built, skipping..."
    fi
}

configure_cmake() {
    print_step "Configuring CMake..."
    
    local build_dir="build-${BUILD_TYPE}"
    if [ "$TARGET_ARCH" != "amd64" ]; then
        build_dir="build-${BUILD_TYPE}-${TARGET_ARCH}"
    fi
    
    if [ "$CLEAN" = true ] && [ -d "$build_dir" ]; then
        print_step "Cleaning build directory..."
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
    
    cmake -DCMAKE_BUILD_TYPE=$cmake_build_type \
          -DTARGET_ARCH=$TARGET_ARCH \
          -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
          -DBUILD_TESTING=ON \
          $CMAKE_ARGS \
          ..
    
    cd ..
    export BUILD_DIR="$build_dir"
    
    print_success "CMake configured successfully"
}

build_project() {
    print_step "Building AASDK..."
    
    cd "$BUILD_DIR"
    
    # Build with progress indicator
    if [ -t 1 ]; then
        # Terminal output - show progress
        make -j$JOBS
    else
        # Non-terminal output - suppress progress for cleaner logs
        make -j$JOBS --no-print-directory
    fi
    
    cd ..
    
    print_success "AASDK built successfully"
}

run_tests() {
    if [ "$RUN_TESTS" = true ]; then
        print_step "Running tests..."
        
        cd "$BUILD_DIR"
        
        if ! ctest --output-on-failure --parallel $JOBS; then
            print_error "Some tests failed"
            cd ..
            exit 1
        fi
        
        cd ..
        
        print_success "All tests passed"
    fi
}

install_project() {
    if [ "$INSTALL" = true ]; then
        if [ "$DRY_RUN" = true ] || [ "$DRY_RUN" = 1 ]; then
            print_step "Dry run enabled: skipping system installation"
            return 0
        fi
        print_step "Installing AASDK..."
        
        cd "$BUILD_DIR"
        
        if [ "$EUID" -eq 0 ]; then
            make install
        else
            sudo make install
            sudo ldconfig
        fi
        
        cd ..
        
        print_success "AASDK installed successfully"
    fi
}

create_packages() {
    if [ "$CREATE_PACKAGES" = true ]; then
        print_step "Creating packages..."
        
        cd "$BUILD_DIR"
        
        # Create DEB packages
        cpack --config CPackConfig.cmake
        
        # Move packages to top-level packages directory
        mkdir -p ../packages
        mv *.deb ../packages/ 2>/dev/null || true
        mv *.tar.* ../packages/ 2>/dev/null || true
        
        cd ..
        
        print_success "Packages created in packages/ directory"
        
        # List created packages
        if [ -d "packages" ]; then
            echo -e "${BLUE}Created packages:${NC}"
            ls -la packages/
        fi
    fi
}

validate_build() {
    print_step "Validating build..."
    
    local lib_file="$BUILD_DIR/lib/libaasdk.so"
    
    if [ ! -f "$lib_file" ]; then
        print_error "Library file not found: $lib_file"
        exit 1
    fi
    
    # Check if library has expected symbols
    if ! nm "$lib_file" | grep -q "aasdk"; then
        print_error "Library does not contain expected AASDK symbols"
        exit 1
    fi
    
    # Check library dependencies (only for native builds)
    if [ "$TARGET_ARCH" = "amd64" ]; then
        if ! ldd "$lib_file" > /dev/null 2>&1; then
            print_error "Library has unresolved dependencies"
            exit 1
        fi
    fi
    
    print_success "Build validation passed"
}

show_build_summary() {
    echo
    echo -e "${BLUE}================================================${NC}"
    echo -e "${GREEN}🎉 AASDK Build Completed Successfully!${NC}"
    echo -e "${BLUE}================================================${NC}"
    echo -e "Build Type:     ${GREEN}${BUILD_TYPE}${NC}"
    echo -e "Architecture:   ${GREEN}${TARGET_ARCH}${NC}"
    echo -e "Build Directory: ${GREEN}${BUILD_DIR}${NC}"
    echo
    echo -e "${BLUE}Build artifacts:${NC}"
    echo -e "  Library:      ${BUILD_DIR}/lib/libaasdk.so"
    echo -e "  Headers:      ${BUILD_DIR}/include/"
    echo -e "  CMake Config: ${BUILD_DIR}/aasdkConfig.cmake"
    echo
    if [ "$RUN_TESTS" = true ]; then
        echo -e "${GREEN}✅ Tests: PASSED${NC}"
    fi
    if [ "$INSTALL" = true ]; then
        echo -e "${GREEN}✅ Installation: COMPLETED${NC}"
    fi
    if [ "$CREATE_PACKAGES" = true ]; then
        echo -e "${GREEN}✅ Packages: CREATED${NC}"
    fi
    echo
    echo -e "${YELLOW}Next steps:${NC}"
    echo -e "  • To run tests: cd ${BUILD_DIR} && ctest"
    echo -e "  • To install: cd ${BUILD_DIR} && sudo make install"
    echo -e "  • To create packages: cd ${BUILD_DIR} && cpack"
    echo -e "  • For troubleshooting: see TROUBLESHOOTING.md"
    echo -e "${BLUE}================================================${NC}"
}

show_usage() {
    echo "AASDK Build Script"
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
    echo "  JOBS=4 $0 debug clean     # Build with 4 parallel jobs"
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
    
    # Build process
    check_dependencies
    setup_cross_compilation
    # NOTE: aap_protobuf is built via add_subdirectory(protobuf) inside the main CMake build.
    # Prebuilding and installing it separately is unnecessary and can require elevated privileges.
    # The previous step has been disabled to keep dry runs Pi-safe and rootless.
    configure_cmake
    build_project
    validate_build
    run_tests
    install_project
    create_packages
    
    show_build_summary
}

# Execute main function
main "$@"
