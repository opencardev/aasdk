# AASDK Comprehensive Documentation

Welcome to the complete documentation for AASDK (Android Auto Software Development Kit), a C++ library implementing core AndroidAuto™ functionalities for building headunit software.

## Table of Contents

1. [Quick Start](#quick-start)
2. [Project Overview](#project-overview)
3. [Development Environment Setup](#development-environment-setup)
4. [Building the Project](#building-the-project)
5. [Testing](#testing)
6. [Packaging and Distribution](#packaging-and-distribution)
7. [Troubleshooting](#troubleshooting)
8. [Contributing](#contributing)
9. [Architecture and Design](#architecture-and-design)
10. [API Reference](#api-reference)

---

## Quick Start

### For Immediate Development

**Using DevContainers (Recommended):**
```bash
# 1. Open VS Code in project directory
# 2. Ctrl+Shift+P → "Dev Containers: Reopen in Container"
# 3. Build the project
./build.sh debug
```

**Traditional Setup:**
```bash
# Install dependencies (Ubuntu/Debian)
sudo apt update
sudo apt install build-essential cmake libboost-all-dev libusb-1.0-0-dev libssl-dev libprotobuf-dev protobuf-compiler

# Build
mkdir build && cd build
cmake ..
make -j$(nproc)
```

---

## Project Overview

### What is AASDK?

AASDK is a comprehensive C++ library that implements the AndroidAuto™ protocol stack, enabling developers to create headunit software that can communicate with Android devices running Android Auto.

### Supported Features

**Core Protocols:**
- ✅ AOAP (Android Open Accessory Protocol)
- ✅ USB transport with hotplug support
- ✅ TCP transport for wireless connectivity
- ✅ SSL encryption for secure communication
- ✅ AndroidAuto™ protocol implementation

**Communication Channels:**
- 🎵 **Media Audio Channel** - Music and audio streaming
- 🔊 **System Audio Channel** - Navigation and system sounds
- 🎤 **Speech Audio Channel** - Voice commands and TTS
- 📞 **Audio Input Channel** - Microphone input
- 📺 **Video Channel** - Display mirroring and apps
- 📱 **Bluetooth Channel** - Bluetooth device management
- 📊 **Sensor Channel** - Vehicle sensor data
- ⚙️ **Control Channel** - System control and configuration
- 🖱️ **Input Channel** - Touch and button inputs

### Architecture Overview

```
┌─────────────────────────────────────────────────────────────────┐
│                        AASDK Library                           │
├─────────────────────────────────────────────────────────────────┤
│  Transport Layer                                               │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐            │
│  │     USB     │  │     TCP     │  │  Bluetooth  │            │
│  └─────────────┘  └─────────────┘  └─────────────┘            │
├─────────────────────────────────────────────────────────────────┤
│  Protocol Layer                                                │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐            │
│  │    AOAP     │  │ AndroidAuto │  │     SSL     │            │
│  └─────────────┘  └─────────────┘  └─────────────┘            │
├─────────────────────────────────────────────────────────────────┤
│  Channel Management                                            │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐            │
│  │    Audio    │  │    Video    │  │   Control   │            │
│  └─────────────┘  └─────────────┘  └─────────────┘            │
└─────────────────────────────────────────────────────────────────┘
```

---

## Development Environment Setup

### Option 1: DevContainers (Recommended)

**Prerequisites:**
- [Docker Desktop](https://www.docker.com/products/docker-desktop/) 
- [VS Code](https://code.visualstudio.com/) with [Dev Containers extension](https://marketplace.visualstudio.com/items?itemName=ms-vscode-remote.remote-containers)

**Supported Architectures:**

| Architecture | Use Case | Container |
|-------------|----------|-----------|
| **x64** | Native development, fastest builds | Default container |
| **ARM64** | Raspberry Pi 4, Apple Silicon | `devcontainer-arm64.json` |
| **ARMHF** | Raspberry Pi 3, older ARM devices | `devcontainer-armhf.json` |

**Setup Steps:**
1. Open VS Code in project directory
2. Press `Ctrl+Shift+P` → "Dev Containers: Reopen in Container"
3. Select target architecture if needed
4. Wait for container setup (5-10 minutes first time)

### Option 2: Native Development

**Ubuntu/Debian Dependencies:**
```bash
sudo apt update
sudo apt install -y \
    build-essential \
    cmake \
    git \
    pkg-config \
    libboost-all-dev \
    libusb-1.0-0-dev \
    libssl-dev \
    libprotobuf-dev \
    protobuf-compiler \
    googletest \
    libgtest-dev
```

**CentOS/RHEL/Fedora Dependencies:**
```bash
sudo dnf install -y \
    gcc gcc-c++ \
    cmake \
    git \
    pkgconfig \
    boost-devel \
    libusb1-devel \
    openssl-devel \
    protobuf-devel \
    protobuf-compiler \
    gtest-devel
```

---

## Building the Project

### Build Scripts (Recommended)

**Quick Build:**
```bash
# Debug build
./build.sh debug

# Release build  
./build.sh release

# Clean and rebuild
./build.sh debug clean
```

### VS Code Tasks

Available tasks (Ctrl+Shift+P → "Tasks: Run Task"):
- `DevContainer: Build Debug (Quick)`
- `DevContainer: Build Release (Quick)` 
- `DevContainer: Clean & Build`
- `DevContainer: Create Packages`

### Manual CMake Build

**Debug Build:**
```bash
mkdir -p build-debug
cd build-debug
cmake -DCMAKE_BUILD_TYPE=Debug -DTARGET_ARCH=${TARGET_ARCH:-amd64} ../
make -j$(nproc)
```

**Release Build:**
```bash
mkdir -p build-release
cd build-release
cmake -DCMAKE_BUILD_TYPE=Release -DTARGET_ARCH=${TARGET_ARCH:-amd64} ../
make -j$(nproc)
```

**Cross-Compilation:**
```bash
# For ARM64
cmake -DCMAKE_BUILD_TYPE=Release -DTARGET_ARCH=arm64 ../

# For ARMHF  
cmake -DCMAKE_BUILD_TYPE=Release -DTARGET_ARCH=armhf ../
```

### Build Options

| Option | Description | Default |
|--------|-------------|---------|
| `CMAKE_BUILD_TYPE` | Build configuration | `Release` |
| `TARGET_ARCH` | Target architecture | `amd64` |
| `BUILD_TESTING` | Enable unit tests | `ON` |
| `ENABLE_COVERAGE` | Enable code coverage | `OFF` |

---

## Testing

### Unit Tests

**Run All Tests:**
```bash
cd build-debug
ctest --output-on-failure
```

**Run Specific Test Suite:**
```bash
cd build-debug
./test_aasdk_transport
./test_aasdk_channel
./test_aasdk_protocol
```

### Integration Tests

**Logger Test:**
```bash
# Compile test program
g++ -std=c++17 test_logger.cpp -o test_logger \
    -I./include \
    -L./build-debug/lib \
    -laasdk \
    -lboost_system \
    -lboost_log \
    -lpthread

# Run logger test
./test_logger
```

**Migration Test:**
```bash
# Test modern logger migration
g++ -std=c++17 test_migration.cpp -o test_migration \
    -I./include \
    -L./build-debug/lib \
    -laasdk

./test_migration
```

### Package Testing

**Test Package Dependencies:**
```bash
cd test_package_build
./test_package_deps.sh
```

**Test Package Installation:**
```bash
# Test runtime package
sudo dpkg -i packages/libaasdk-*.deb

# Test development package  
sudo dpkg -i packages/libaasdk-*-dev*.deb

# Verify installation
ldconfig -p | grep aasdk
pkg-config --modversion aasdk
```

### Performance Testing

**Memory Leak Detection:**
```bash
valgrind --tool=memcheck --leak-check=full ./your_aasdk_app
```

**Performance Profiling:**
```bash
perf record -g ./your_aasdk_app
perf report
```

---

## Packaging and Distribution

### Package Types

The build system creates semantic versioned packages:

**Release Packages:**
- `libaasdk-{arch}_{version}_{arch}.deb` - Runtime library
- `libaasdk-{arch}-dev_{version}_{arch}.deb` - Development headers

**Debug Packages:**
- `libaasdk-{arch}-dbg_{version}.debug_{arch}.deb` - Debug runtime
- `libaasdk-{arch}-dbg-dev_{version}.debug_{arch}.deb` - Debug development

**Source Package:**
- `libaasdk-{arch}-{version}-src.tar.gz` - Source distribution

### Version Format

Date-based semantic versioning: `YYYY.MM.DD+git.{commit}.{status}`

Examples:
- `2025.07.30+git.abc1234` (clean release)
- `2025.07.30+git.abc1234.dirty` (uncommitted changes)
- `2025.07.30+git.abc1234.debug` (debug build)

### Creating Packages

**Using Tasks:**
```bash
# VS Code: Ctrl+Shift+P → "Tasks: Run Task" → "DevContainer: Create Packages"
```

**Manual Creation:**
```bash
# Release packages
cd build-release
cpack --config CPackConfig.cmake
mkdir -p ../packages
mv *.deb ../packages/

# Debug packages
cd build-debug  
cpack --config CPackConfig.cmake
mkdir -p ../packages
mv *.deb ../packages/

# Source package
cd build-release
cpack --config CPackSourceConfig.cmake
mv *.tar.* ../packages/
```

---

## Troubleshooting

### Common Build Issues

#### 1. Missing Dependencies

**Problem:** CMake cannot find required libraries
```
CMake Error: Could not find Boost
```

**Solution:**
```bash
# Ubuntu/Debian
sudo apt install libboost-all-dev

# CentOS/RHEL/Fedora
sudo dnf install boost-devel
```

#### 2. Protobuf Version Mismatch

**Problem:** 
```
error: 'GOOGLE_PROTOBUF_VERSION' was not declared
```

**Solution:**
```bash
# Check protobuf version
protoc --version

# Install correct version (Ubuntu)
sudo apt install libprotobuf-dev protobuf-compiler

# Build protobuf from source if needed
cd protobuf
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc) && make install
```

#### 3. USB Library Issues

**Problem:**
```
fatal error: libusb.h: No such file or directory
```

**Solution:**
```bash
# Install libusb development headers
sudo apt install libusb-1.0-0-dev
```

#### 4. Cross-Compilation Issues

**Problem:** ARM cross-compilation fails
```
No CMAKE_C_COMPILER could be found
```

**Solution:**
```bash
# Install cross-compilation toolchain
sudo apt install gcc-arm-linux-gnueabihf g++-arm-linux-gnueabihf
sudo apt install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu

# Verify toolchain
arm-linux-gnueabihf-gcc --version
aarch64-linux-gnu-gcc --version
```

### Runtime Issues

#### 1. USB Permission Denied

**Problem:** Cannot access USB devices
```
libusb: error [get_usbfs_fd] libusb couldn't open USB device /dev/bus/usb/001/002: Permission denied
```

**Solution:**
```bash
# Add user to plugdev group
sudo usermod -a -G plugdev $USER

# Create udev rule for Android devices
sudo tee /etc/udev/rules.d/51-android.rules << EOF
SUBSYSTEM=="usb", ATTR{idVendor}=="18d1", MODE="0666", GROUP="plugdev"
SUBSYSTEM=="usb", ATTR{idVendor}=="22b8", MODE="0666", GROUP="plugdev"
EOF

# Reload udev rules
sudo udevadm control --reload-rules
sudo udevadm trigger
```

#### 2. SSL Certificate Issues

**Problem:** SSL handshake failures
```
SSL_connect failed: certificate verify failed
```

**Solution:**
```bash
# Update CA certificates
sudo apt update && sudo apt install ca-certificates

# Check SSL configuration in your code
// Verify SSL context setup in Transport layer
```

#### 3. Audio Issues

**Problem:** No audio output
```
ALSA lib pcm_dmix.c: unable to open slave
```

**Solution:**
```bash
# Check ALSA configuration
aplay -l
amixer scontents

# Install PulseAudio if needed
sudo apt install pulseaudio pulseaudio-utils

# Test audio
speaker-test -t wav -c 2
```

### Debugging Tools

#### 1. Enable Debug Logging

```cpp
#include <aasdk/Common/Log.hpp>

// Set log level to DEBUG
aasdk::common::Log::setLogLevel(aasdk::common::LogLevel::DEBUG);

// Use category-specific logging
AASDK_LOG_USB_DEBUG() << "USB debug message";
AASDK_LOG_TRANSPORT_ERROR() << "Transport error occurred";
```

#### 2. Network Debugging

```bash
# Monitor TCP connections
netstat -an | grep :5277

# Capture network traffic
sudo tcpdump -i any port 5277 -w androidauto.pcap

# Analyze with Wireshark
wireshark androidauto.pcap
```

#### 3. USB Debugging

```bash
# List USB devices
lsusb -v

# Monitor USB events
sudo udevadm monitor --udev

# Check USB permissions
ls -la /dev/bus/usb/
```

### Performance Issues

#### 1. High CPU Usage

**Check:**
- Audio processing threads
- Video decoding performance
- Logging verbosity

**Solution:**
```cpp
// Reduce logging verbosity
aasdk::common::Log::setLogLevel(aasdk::common::LogLevel::INFO);

// Optimize audio buffer sizes
// Implement proper thread priority
```

#### 2. Memory Leaks

**Detection:**
```bash
# Compile with AddressSanitizer
cmake -DCMAKE_CXX_FLAGS="-fsanitize=address" ..

# Use Valgrind
valgrind --tool=memcheck --leak-check=full ./your_app
```

#### 3. Threading Issues

**Debug Race Conditions:**
```bash
# Compile with ThreadSanitizer
cmake -DCMAKE_CXX_FLAGS="-fsanitize=thread" ..

# Use GDB for debugging
gdb ./your_app
(gdb) set scheduler-locking step
(gdb) thread apply all bt
```

---

## Contributing

### Development Workflow

1. **Fork and Clone:**
   ```bash
   git clone https://github.com/opencardev/aasdk.git
   cd aasdk
   ```

2. **Create Feature Branch:**
   ```bash
   git checkout -b feature/your-feature-name
   ```

3. **Set Up Development Environment:**
   ```bash
   # Use DevContainer or install dependencies natively
   ./build.sh debug
   ```

4. **Make Changes and Test:**
   ```bash
   # Build and test your changes
   ./build.sh debug
   cd build-debug && ctest
   ```

5. **Submit Pull Request:**
   - Ensure all tests pass
   - Follow coding standards
   - Update documentation if needed

### Coding Standards

**C++ Style:**
- Follow Google C++ Style Guide
- Use RAII principles
- Prefer smart pointers over raw pointers
- Use const-correctness

**Logging:**
- Use modern logger categories for new code
- Include context information in log messages
- Use appropriate log levels (DEBUG/INFO/WARN/ERROR)

**Testing:**
- Write unit tests for new functionality
- Ensure high code coverage
- Test on multiple architectures

### Modern Logger Migration

For migrating existing code to use the modern logger:

```bash
# Use the migration script
./migrate_to_modern_logger.sh /path/to/your/source/files
```

**Before:**
```cpp
AASDK_LOG(debug) << "Connection established";
```

**After:**
```cpp
AASDK_LOG_TRANSPORT_DEBUG() << "Connection established";
```

---

## Architecture and Design

### Module Structure

```
aasdk/
├── Common/         # Core utilities and logging
├── Error/          # Error handling and exceptions  
├── IO/             # I/O operations and buffers
├── Transport/      # USB and TCP transport layers
├── Channel/        # Communication channels
├── Messenger/      # Message serialization/deserialization
├── USB/            # USB-specific implementations
└── TCP/            # TCP-specific implementations
```

### Design Patterns

**Factory Pattern:** Used for creating transport and channel instances
**Observer Pattern:** Used for event notifications
**Strategy Pattern:** Used for different audio/video codec implementations
**RAII:** Used throughout for resource management

### Threading Model

- **Main Thread:** UI and control operations
- **Transport Thread:** USB/TCP communication
- **Audio Thread:** Real-time audio processing
- **Video Thread:** Video frame processing
- **Worker Threads:** Background processing tasks

---

## API Reference

### Core Classes

#### Transport Layer

```cpp
namespace aasdk::transport {
    class ITransport {
    public:
        virtual void start() = 0;
        virtual void stop() = 0;
        virtual void send(const common::DataBuffer& buffer) = 0;
    };
    
    class USBTransport : public ITransport {
        // USB-specific implementation
    };
    
    class TCPTransport : public ITransport {
        // TCP-specific implementation  
    };
}
```

#### Channel Management

```cpp
namespace aasdk::channel {
    class IChannel {
    public:
        virtual void start() = 0;
        virtual void stop() = 0;
        virtual ChannelId getId() const = 0;
    };
    
    class AudioChannel : public IChannel {
        // Audio channel implementation
    };
    
    class VideoChannel : public IChannel {
        // Video channel implementation
    };
}
```

#### Logging System

```cpp
namespace aasdk::common {
    enum class LogLevel {
        TRACE, DEBUG, INFO, WARN, ERROR, FATAL
    };
    
    enum class LogCategory {
        SYSTEM, TRANSPORT, USB, TCP, AUDIO, VIDEO, 
        CHANNEL, MESSENGER, PROTOCOL, GENERAL
    };
    
    class ModernLogger {
    public:
        static void initialize();
        static void setLevel(LogLevel level);
        static void setLevel(LogCategory category, LogLevel level);
    };
}
```

### Usage Examples

#### Basic Transport Setup

```cpp
#include <aasdk/Transport/USBTransport.hpp>
#include <aasdk/Common/Log.hpp>

// Initialize logging
aasdk::common::ModernLogger::initialize();

// Create USB transport
auto transport = std::make_unique<aasdk::transport::USBTransport>();

// Set up event handlers
transport->setReceiveHandler([](const aasdk::common::DataBuffer& buffer) {
    AASDK_LOG_TRANSPORT_INFO() << "Received " << buffer.size() << " bytes";
});

// Start transport
transport->start();
```

#### Audio Channel Implementation

```cpp
#include <aasdk/Channel/AudioChannel.hpp>

auto audioChannel = std::make_unique<aasdk::channel::AudioChannel>(transport);

audioChannel->setAudioHandler([](const aasdk::audio::AudioData& data) {
    AASDK_LOG_AUDIO_MEDIA_DEBUG() << "Processing audio frame: " << data.size();
    // Process audio data
});

audioChannel->start();
```

---

## Additional Resources

### External Documentation

- **AndroidAuto Protocol:** [Official Documentation](https://source.android.com/devices/automotive)
- **Protocol Buffers:** [Google Protobuf Guide](https://developers.google.com/protocol-buffers)
- **Boost Libraries:** [Boost Documentation](https://www.boost.org/doc/)
- **CMake:** [CMake Tutorial](https://cmake.org/cmake/help/latest/guide/tutorial/)

### Related Projects

- **OpenDsh:** Android Auto headunit implementation using AASDK
- **AAuto:** Desktop Android Auto client
- **Crankshaft:** Raspberry Pi Android Auto solution

### Community

- **GitHub Issues:** [Report bugs and request features](https://github.com/opencardev/aasdk/issues)
- **Discussions:** [Community discussions](https://github.com/opencardev/aasdk/discussions)
- **Matrix Chat:** [Join our chat room](https://matrix.to/#/#aasdk:matrix.org)

---

## License

GNU GPLv3

Copyrights (c) 2018 f1x.studio (Michal Szwaj)

*AndroidAuto is registered trademark of Google Inc.*

---

## Changelog

### Latest Changes

- ✅ Modern logging system with 47+ specialized macros
- ✅ Multi-architecture DevContainer support (x64, ARM64, ARMHF)
- ✅ Semantic date-based versioning system
- ✅ Comprehensive package building and distribution
- ✅ Enhanced testing and debugging capabilities
- ✅ Improved cross-compilation support

### Migration Notes

- Legacy logging syntax remains fully supported
- New projects should use modern logger categories
- Package versioning has changed to date-based format
- DevContainer is now the recommended development method

---

*This documentation is maintained by the AASDK community. For updates and corrections, please submit pull requests or open issues on GitHub.*
