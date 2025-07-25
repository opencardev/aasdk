#!/bin/bash

echo "🚀 Setting up AASDK development environment..."

# Get the target architecture from environment
TARGET_ARCH=${TARGET_ARCH:-amd64}
echo "📦 Target architecture: $TARGET_ARCH"

# Create build directories
echo "📁 Creating build directories..."
mkdir -p /workspace/build-debug
mkdir -p /workspace/build-release
mkdir -p /workspace/packages

# Set up protobuf build directory
echo "🔧 Setting up protobuf build directory..."
mkdir -p /workspace/protobuf/build

# Install additional dependencies based on architecture
if [ "$TARGET_ARCH" = "armhf" ]; then
    echo "🔧 Installing ARMHF-specific dependencies..."
    # Additional ARMHF dependencies can be added here
    export CC=/usr/bin/arm-linux-gnueabihf-gcc
    export CXX=/usr/bin/arm-linux-gnueabihf-g++
elif [ "$TARGET_ARCH" = "arm64" ]; then
    echo "🔧 Installing ARM64-specific dependencies..."
    # Additional ARM64 dependencies can be added here
    export CC=/usr/bin/aarch64-linux-gnu-gcc
    export CXX=/usr/bin/aarch64-linux-gnu-g++
else
    echo "🔧 Using native x64 toolchain..."
fi

# Configure git safe directory
echo "🔐 Configuring git safe directory..."
git config --global --add safe.directory /workspace

# Set up CMake presets if they don't exist
echo "📋 Setting up build environment..."

# Create a simple build script
cat > /workspace/build.sh << 'EOF'
#!/bin/bash

# AASDK Build Script
# Usage: ./build.sh [debug|release] [clean]

BUILD_TYPE=${1:-debug}
CLEAN=${2}

if [ "$CLEAN" = "clean" ]; then
    echo "🧹 Cleaning build directories..."
    rm -rf build-debug build-release protobuf/build
    mkdir -p build-debug build-release protobuf/build
fi

echo "🏗️  Building AASDK for $TARGET_ARCH in $BUILD_TYPE mode..."

# Build protobuf first
echo "📦 Building protobuf..."
cd protobuf/build
cmake -DCMAKE_BUILD_TYPE=Release -DTARGET_ARCH=$TARGET_ARCH ..
make -j$(nproc)
make install
cd ../..

# Build main project
BUILD_DIR="build-$BUILD_TYPE"
echo "🔨 Building main project in $BUILD_DIR..."

cd $BUILD_DIR
cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DTARGET_ARCH=$TARGET_ARCH ..
make -j$(nproc)

echo "✅ Build complete!"
EOF

chmod +x /workspace/build.sh

# Create a package script
cat > /workspace/package.sh << 'EOF'
#!/bin/bash

# AASDK Package Script
# Usage: ./package.sh [release|debug]

BUILD_TYPE=${1:-release}
BUILD_DIR="build-$BUILD_TYPE"

echo "📦 Creating $BUILD_TYPE packages for $TARGET_ARCH..."

if [ ! -d "$BUILD_DIR" ]; then
    echo "❌ Build directory $BUILD_DIR not found. Run build.sh first."
    exit 1
fi

cd $BUILD_DIR
cpack --config CPackConfig.cmake
mkdir -p ../packages
mv *.deb ../packages/ 2>/dev/null || true
mv *.tar.* ../packages/ 2>/dev/null || true

echo "✅ Packages created in packages/ directory"
EOF

chmod +x /workspace/package.sh

echo "✅ AASDK development environment setup complete!"
echo ""
echo "🎯 Quick start commands:"
echo "  ./build.sh debug     - Build debug version"
echo "  ./build.sh release   - Build release version"
echo "  ./package.sh release - Create release packages"
echo ""
echo "📖 Use VS Code CMake Tools extension for integrated development"
