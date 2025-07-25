
# aasdk

[![Contributor Covenant](https://img.shields.io/badge/Contributor%20Covenant-2.1-4baaaa.svg)](code_of_conduct.md)

### Brief description
C++ object-oriented library containing implementation of core AndroidAuto(tm) functionalities needed to build headunit software.

### Changes
Added recent changes by the OpenDsh team

## 🚀 Quick Start with Dev Containers

**New!** This project now includes a comprehensive development environment using VS Code devcontainers that supports building for **x64**, **ARM64**, and **ARMHF** architectures.

### Prerequisites
- [Docker Desktop](https://www.docker.com/products/docker-desktop/)
- [VS Code](https://code.visualstudio.com/) with [Dev Containers extension](https://marketplace.visualstudio.com/items?itemName=ms-vscode-remote.remote-containers)

### Getting Started
1. Open VS Code in this directory
2. Press `Ctrl+Shift+P` → "Dev Containers: Reopen in Container"
3. Choose your target architecture:
   - **Default (x64)**: Fastest for development
   - **ARM64**: For Raspberry Pi 4, Apple Silicon
   - **ARMHF**: For Raspberry Pi 3, older ARM devices
4. Wait for setup, then run: `./build.sh debug`

📖 **[Complete DevContainer Documentation](DEVCONTAINER.md)**

### Supported functionalities
 - AOAP (Android Open Accessory Protocol)
 - USB transport
 - TCP transport
 - USB hotplug
 - AndroidAuto(tm) protocol
 - SSL encryption

### Supported AndroidAuto(tm) communication channels
 - Media audio channel
 - System audio channel
 - Speech audio channel
 - Audio input channel
 - Video channel
 - Bluetooth channel
 - Sensor channel
 - Control channel
 - Input channel

### License
GNU GPLv3

Copyrights (c) 2018 f1x.studio (Michal Szwaj)

*AndroidAuto is registered trademark of Google Inc.*

### Used software
 - [Boost libraries](http://www.boost.org/)
 - [libusb](http://libusb.info/)
 - [CMake](https://cmake.org/)
 - [Protocol buffers](https://developers.google.com/protocol-buffers/)
 - [OpenSSL](https://www.openssl.org/)
 - [Google test framework](https://github.com/google/googletest)
