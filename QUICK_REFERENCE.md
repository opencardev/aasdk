# AASDK Quick Reference

**Essential commands and information for AASDK development.**

## 🚀 Quick Commands

### Build & Development
```bash
# Linux/macOS/DevContainer
./build.sh debug                    # Debug build
./build.sh release                  # Release build
./build.sh debug clean             # Clean debug build
./build.sh release test            # Release build with tests
./build.sh debug package           # Debug build with packages

# Windows PowerShell
.\build.ps1 debug                   # Debug build
.\build.ps1 release clean          # Clean release build
.\build.ps1 -BuildType debug test  # Debug build with tests

# Windows Batch (wrapper)
.\build.bat debug                   # Uses available Unix environment
.\build.bat release clean          # Clean release build

# DevContainer (all platforms)
# Ctrl+Shift+P → "Dev Containers: Reopen in Container"
./build.sh debug                    # Works on all platforms in container

# Native Build (manual)
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)

# Cross-compilation
TARGET_ARCH=arm64 ./build.sh release
TARGET_ARCH=armhf ./build.sh release
```

### Testing
```bash
cd build-debug
ctest --output-on-failure          # Run all tests
ctest -R "transport"               # Run transport tests
./test_aasdk_common                # Run specific test

# Memory checking
valgrind --leak-check=full ./test_program
```

### Packaging
```bash
cd build-release
cpack --config CPackConfig.cmake   # Create packages
sudo dpkg -i ../packages/*.deb     # Install package
```

## 🔍 Troubleshooting Quick Fixes

### Build Issues
```bash
# Missing dependencies
sudo apt install libboost-all-dev libusb-1.0-0-dev libssl-dev protobuf-compiler

# Clean build
rm -rf build-* && ./build.sh debug clean

# Protobuf issues
cd protobuf && mkdir build && cd build
cmake .. && make -j$(nproc) && make install
```

### Runtime Issues
```bash
# USB permissions
sudo usermod -a -G plugdev $USER
sudo tee /etc/udev/rules.d/51-android.rules << 'EOF'
SUBSYSTEM=="usb", ATTR{idVendor}=="18d1", MODE="0666", GROUP="plugdev"
EOF
sudo udevadm control --reload-rules

# Network issues
sudo ufw allow 5277               # Allow AndroidAuto port
```

## 📝 Code Examples

### Basic Logger Usage
```cpp
#include <aasdk/Common/Log.hpp>

// Modern logging (recommended)
AASDK_LOG_TRANSPORT_INFO() << "Connection established";
AASDK_LOG_USB_DEBUG() << "Device detected: " << deviceInfo;
AASDK_LOG_AUDIO_MEDIA_WARN() << "Buffer underrun detected";

// Legacy logging (still supported)
AASDK_LOG(info) << "Legacy log message";
```

### Transport Setup
```cpp
// USB Transport
auto usbTransport = std::make_unique<aasdk::transport::USBTransport>();
usbTransport->start();

// TCP Transport
auto tcpTransport = std::make_unique<aasdk::transport::TCPTransport>();
tcpTransport->bind("0.0.0.0", 5277);
```

## 📊 Architecture Support

| Architecture | CMake Target | DevContainer | Use Case |
|-------------|-------------|--------------|----------|
| **x64** | `amd64` | Default | Development, servers |
| **ARM64** | `arm64` | `arm64` | Raspberry Pi 4 |
| **ARMHF** | `armhf` | `armhf` | Raspberry Pi 3 |

## 🎯 Documentation Quick Links

| Need | Document | Section |
|------|----------|---------|
| **Getting Started** | [DOCUMENTATION.md](DOCUMENTATION.md) | Quick Start |
| **Build Problems** | [BUILD.md](BUILD.md) | Prerequisites |
| **Runtime Issues** | [TROUBLESHOOTING.md](TROUBLESHOOTING.md) | Runtime Issues |
| **Test Failures** | [TESTING.md](TESTING.md) | Unit Testing |
| **DevContainer Setup** | [DEVCONTAINER.md](DEVCONTAINER.md) | Quick Start |
| **Package Creation** | [PACKAGING.md](PACKAGING.md) | Package Types |

## 🔧 Environment Variables

```bash
# Build configuration
export TARGET_ARCH=arm64           # Cross-compilation target
export CMAKE_BUILD_PARALLEL_LEVEL=4 # Parallel build jobs
export AASDK_LOG_LEVEL=DEBUG       # Logging level

# Library paths
export PKG_CONFIG_PATH="/usr/local/lib/pkgconfig:$PKG_CONFIG_PATH"
export LD_LIBRARY_PATH="/usr/local/lib:$LD_LIBRARY_PATH"
```

## 📦 Package Information

### Version Format
`YYYY.MM.DD+git.{commit}.{status}`

Examples:
- `2025.07.30+git.abc1234` (clean)
- `2025.07.30+git.abc1234.dirty` (uncommitted)
- `2025.07.30+git.abc1234.debug` (debug)

### Package Types
- **libaasdk** - Runtime library
- **libaasdk-dev** - Development headers
- **libaasdk-dbg** - Debug symbols

## 🚨 Emergency Procedures

### Complete Clean Build
```bash
#!/bin/bash
rm -rf build-*
cd protobuf && rm -rf build && cd ..
find . -name "CMakeCache.txt" -delete
./build.sh debug clean
```

### Diagnostic Information
```bash
# System info
uname -a
gcc --version
cmake --version

# Dependencies
pkg-config --modversion protobuf
dpkg -l | grep boost

# Build status
ls -la build-*/lib/
ldd build-debug/lib/libaasdk.so
```

## 📞 Getting Help

1. **Documentation:** Start with [DOCUMENTATION.md](DOCUMENTATION.md)
2. **Issues:** [GitHub Issues](https://github.com/opencardev/aasdk/issues)
3. **Community:** [GitHub Discussions](https://github.com/opencardev/aasdk/discussions)
4. **Search:** Use Ctrl+F in documentation files

## 🎯 Common Use Cases

### Raspberry Pi Development
```bash
# Use ARM64 DevContainer
# Ctrl+Shift+P → "Dev Containers: Reopen in Container" → arm64
./build.sh release
cd build-release && cpack
```

### Performance Testing
```bash
# Build with optimizations
./build.sh release

# Profile performance
perf record -g ./your_app
perf report

# Memory analysis
valgrind --tool=massif ./your_app
```

### Contributing
```bash
# Development setup
git checkout -b feature/my-feature
# Use DevContainer for consistent environment
./build.sh debug && cd build-debug && ctest
# Submit PR with all tests passing
```

---

*Keep this reference handy for quick access to essential AASDK information.*
