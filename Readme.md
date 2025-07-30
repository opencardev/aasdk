
# AASDK - Android Auto Software Development Kit

[![Contributor Covenant](https://img.shields.io/badge/Contributor%20Covenant-2.1-4baaaa.svg)](code_of_conduct.md)
[![Build Status](https://img.shields.io/badge/build-passing-brightgreen)](BUILD.md)
[![Documentation](https://img.shields.io/badge/docs-comprehensive-blue)](DOCUMENTATION.md)

## Overview

**AASDK** is a comprehensive C++ library implementing the complete AndroidAuto™ protocol stack, enabling developers to create headunit software that seamlessly communicates with Android devices. This library provides all the core functionalities needed to build production-ready AndroidAuto implementations.

### Key Features ✨

- 🚗 **Complete AndroidAuto™ Protocol** - Full implementation of the official protocol
- � **Multi-Transport Support** - USB, TCP, and Bluetooth connectivity
- 🎵 **Rich Media Channels** - Audio, video, input, and control channels
- 🛡️ **Security** - SSL encryption and authentication
- 🏗️ **Multi-Architecture** - x64, ARM64, ARMHF support
- 📦 **Modern Packaging** - DEB packages with semantic versioning
- 🧪 **Comprehensive Testing** - Unit, integration, and performance tests
- 📚 **Extensive Documentation** - Complete guides and troubleshooting

## 🚀 Quick Start

### Option 1: DevContainers (Recommended)

**Prerequisites:**
- [Docker Desktop](https://www.docker.com/products/docker-desktop/)
- [VS Code](https://code.visualstudio.com/) with [Dev Containers extension](https://marketplace.visualstudio.com/items?itemName=ms-vscode-remote.remote-containers)

**Get Started in 3 Steps:**
1. Open VS Code in this directory
2. Press `Ctrl+Shift+P` → "Dev Containers: Reopen in Container"
3. Build: `./build.sh debug`

### Option 2: Native Build

**Ubuntu/Debian Quick Setup:**
```bash
# Install dependencies
sudo apt update && sudo apt install -y build-essential cmake git \
    libboost-all-dev libusb-1.0-0-dev libssl-dev libprotobuf-dev protobuf-compiler

# Build
git clone https://github.com/opencardev/aasdk.git
cd aasdk && mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
```

## 📚 Comprehensive Documentation

| Document | Description |
|----------|-------------|
| **[📖 Complete Documentation](DOCUMENTATION.md)** | **Comprehensive guide covering everything** |
| [🔨 Build Guide](BUILD.md) | Detailed build instructions for all platforms |
| [🧪 Testing Guide](TESTING.md) | Complete testing procedures and validation |
| [🔧 Troubleshooting](TROUBLESHOOTING.md) | Solutions for common issues and debugging |
| [� DevContainer Guide](DEVCONTAINER.md) | Multi-architecture development environment |
| [📦 Packaging Guide](PACKAGING.md) | Package building and distribution |
| [🚀 Modern Logger](MODERN_LOGGER.md) | Advanced logging system documentation |
| [⚡ Implementation Summary](IMPLEMENTATION_SUMMARY.md) | Recent changes and features |

## 🏗️ Supported Architectures

| Architecture | Container | Use Case | Status |
|-------------|-----------|----------|--------|
| **x64** | Default | Development, servers | ✅ Fully supported |
| **ARM64** | arm64 | Raspberry Pi 4, modern ARM | ✅ Fully supported |
| **ARMHF** | armhf | Raspberry Pi 3, legacy ARM | ✅ Fully supported |

## 🔌 Protocol Support

### Transport Layers
- ✅ **USB Transport** - High-speed wired connection with hotplug support
- ✅ **TCP Transport** - Wireless AndroidAuto over WiFi
- ✅ **SSL Encryption** - Secure communication channel

### Communication Channels
- 🎵 **Media Audio** - Music and media streaming
- 🔊 **System Audio** - Navigation prompts and notifications  
- 🎤 **Speech Audio** - Voice commands and assistant
- 📞 **Audio Input** - Microphone and telephony
- 📺 **Video Channel** - App display and mirroring
- 📱 **Bluetooth** - Device pairing and management
- 📊 **Sensor Channel** - Vehicle data integration
- ⚙️ **Control Channel** - System control and configuration
- 🖱️ **Input Channel** - Touch, buttons, and steering wheel controls

## 🛠️ Build Options

### Quick Build Commands
```bash
# Linux/macOS/DevContainer
./build.sh debug           # Development build with debugging
./build.sh release         # Production build with optimizations
./build.sh debug clean     # Clean rebuild
./build.sh release package # Build and create packages

# Windows PowerShell
.\build.ps1 debug          # Development build
.\build.ps1 release clean  # Clean production build

# Windows Batch (auto-detects environment)
.\build.bat debug          # Uses WSL, Git Bash, or shows setup help

# Cross-compilation
TARGET_ARCH=arm64 ./build.sh release   # ARM64 build
TARGET_ARCH=armhf ./build.sh release   # ARMHF build
```

### VS Code Integration
Use integrated tasks for seamless development:
- `Ctrl+Shift+P` → "Tasks: Run Task"
- Select: `DevContainer: Build Debug (Quick)`

### DevContainer Build (Cross-Platform)
```bash
# Works on Windows, macOS, Linux
# 1. Open VS Code in project directory
# 2. Ctrl+Shift+P → "Dev Containers: Reopen in Container"
# 3. Build with single command
./build.sh debug
```

## 🧪 Testing & Validation

**Run Tests:**
```bash
cd build-debug && ctest --output-on-failure
```

**Performance Testing:**
```bash
# Memory leak detection
valgrind --leak-check=full ./your_aasdk_app

# Performance profiling
perf record -g ./your_aasdk_app && perf report
```

**Package Testing:**
```bash
# Create and test DEB packages
./build.sh release
cd build-release && cpack
sudo dpkg -i ../packages/libaasdk*.deb
```

## 📦 Package Distribution

### Available Packages

| Package Type | Description | Use Case |
|-------------|-------------|----------|
| **libaasdk** | Runtime library | Production deployment |
| **libaasdk-dev** | Development headers | Building applications |
| **libaasdk-dbg** | Debug symbols | Development and debugging |
| **Source packages** | Complete source | Distribution and archival |

### Version Format
**Semantic Date-Based Versioning:** `YYYY.MM.DD+git.{commit}.{status}`

Examples:
- `2025.07.30+git.abc1234` (clean release)
- `2025.07.30+git.abc1234.dirty` (uncommitted changes)
- `2025.07.30+git.abc1234.debug` (debug build)

## 🔍 Troubleshooting

### Common Issues Quick Reference

| Issue | Quick Solution | Full Guide |
|-------|---------------|------------|
| **Build errors** | `./build.sh debug clean` | [BUILD.md](BUILD.md) |
| **USB permissions** | Add user to `plugdev` group | [TROUBLESHOOTING.md](TROUBLESHOOTING.md#usb-permission-problems) |
| **Missing dependencies** | Run dependency installer script | [BUILD.md#prerequisites](BUILD.md#prerequisites) |
| **Cross-compilation** | Use DevContainer for target arch | [BUILD.md#cross-compilation](BUILD.md#cross-compilation) |

### Getting Help

1. **Check Documentation:** Start with [DOCUMENTATION.md](DOCUMENTATION.md)
2. **Search Issues:** [GitHub Issues](https://github.com/opencardev/aasdk/issues)
3. **Ask Community:** [GitHub Discussions](https://github.com/opencardev/aasdk/discussions)
4. **Report Bugs:** Use issue templates with diagnostic info

## 🤝 Contributing

We welcome contributions! Here's how to get started:

1. **Read the Guide:** Check [CONTRIBUTING.md](CONTRIBUTING.md)
2. **Set Up Environment:** Use DevContainer for consistent setup
3. **Follow Standards:** Use modern logger categories and coding standards
4. **Test Thoroughly:** Run full test suite before submitting PRs

### Development Workflow
```bash
# 1. Fork and clone
git clone https://github.com/your-fork/aasdk.git

# 2. Create feature branch
git checkout -b feature/amazing-feature

# 3. Set up development environment (DevContainer recommended)
# 4. Make changes and test
./build.sh debug && cd build-debug && ctest

# 5. Submit pull request
```

## 📄 License

**GNU GPLv3** - See [LICENSE](LICENSE) for details

Copyrights (c) 2018 f1x.studio (Michal Szwaj)  
Enhanced by the OpenCarDev community

*AndroidAuto is a registered trademark of Google Inc.*

## 🙏 Acknowledgments

### Core Dependencies
- **[Boost Libraries](http://www.boost.org/)** - C++ utility libraries
- **[libusb](http://libusb.info/)** - USB device access
- **[CMake](https://cmake.org/)** - Build system
- **[Protocol Buffers](https://developers.google.com/protocol-buffers/)** - Data serialization
- **[OpenSSL](https://www.openssl.org/)** - Cryptographic functions
- **[Google Test](https://github.com/google/googletest)** - Unit testing framework

### Community Projects
- **[OpenDsh](https://github.com/opencardev/opendsh)** - Android Auto headunit using AASDK
- **[Crankshaft](https://getcrankshaft.com/)** - Raspberry Pi Android Auto solution

---

## 🚀 Recent Updates

### Version 2025.07.30+ Features
- ✅ **Modern Logging System** - 47+ specialized logging macros
- ✅ **Multi-Architecture DevContainers** - x64, ARM64, ARMHF support
- ✅ **Semantic Versioning** - Date-based package versions
- ✅ **Comprehensive Documentation** - Complete guides and troubleshooting
- ✅ **Enhanced Testing** - Unit, integration, and performance tests
- ✅ **Improved Packaging** - Professional DEB package distribution

### Migration Notes
- Legacy logging syntax remains fully supported
- New projects should use modern logger categories
- DevContainer is now the recommended development method
- Package versioning changed to date-based format

---

**For complete information, start with [📖 DOCUMENTATION.md](DOCUMENTATION.md)**

*Maintained by the AASDK community • [Contribute](CONTRIBUTING.md) • [Report Issues](https://github.com/opencardev/aasdk/issues)*
