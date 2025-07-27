# AASDK Package Building

This project now supports building proper DEB packages for both Release and Debug configurations with semantic date-based versioning.

## Versioning System

The project uses a semantic date-based versioning system:
- **Major Version**: Year (e.g., 2025)
- **Minor Version**: Month (01-12) 
- **Patch Version**: Day (01-31)
- **Build Metadata**: Git commit ID and repository status

### Version Format Examples:
- Release: `2025.07.19+git.4e1de90` (clean repository)
- Release (dirty): `2025.07.19+git.4e1de90.dirty` (uncommitted changes)
- Debug: `2025.07.19+git.4e1de90.debug` (debug build)
- Debug (dirty): `2025.07.19+git.4e1de90.dirty.debug` (debug with uncommitted changes)

## Package Types

The build system creates the following packages:

### Release Packages
- **libaasdk**: Runtime library package containing the shared library
- **libaasdk-dev**: Development package containing headers and CMake configuration files

### Debug Packages
- **libaasdk-dbg**: Debug runtime library package with debugging symbols
- **libaasdk-dbg-dev**: Debug development package

### Source Package
- **libaasdk-src**: Source code package for distribution

## Version Information in Code

The build system generates a `Version.hpp` header file that provides version and build information to the application:

```cpp
#include <aasdk/Version.hpp>

// Access version components
unsigned int major = aasdk::Version::MAJOR;     // 2025
unsigned int minor = aasdk::Version::MINOR;     // 07  
unsigned int patch = aasdk::Version::PATCH;     // 19

// Access full version string
const char* version = aasdk::Version::STRING;   // "2025.07.19+git.4e1de90.dirty"

// Access git information
const char* commit = aasdk::Version::GIT_COMMIT;  // "4e1de90"
const char* branch = aasdk::Version::GIT_BRANCH;  // "newdev"
bool isDirty = aasdk::Version::isDirty();         // true if uncommitted changes

// Helper methods
std::string semantic = aasdk::Version::getSemanticVersion();  // "2025.07.19"
bool isClean = aasdk::Version::isCleanBuild();               // false if dirty
```

## Building Packages

### Using VS Code Tasks

You can use the following VS Code tasks (Ctrl+Shift+P → "Tasks: Run Task"):

1. **Build All Packages**: Builds both release and debug packages plus source package
2. **Create Release DEB Packages**: Creates only release packages
3. **Create Debug DEB Packages**: Creates only debug packages
4. **Install Release Package**: Installs the built release packages
5. **Install Debug Package**: Installs the built debug packages

### Manual Building

#### Release Packages
```bash
# Configure and build
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ../
make -j$(nproc)

# Create packages
cpack --config CPackConfig.cmake
```

#### Debug Packages
```bash
# Configure and build
mkdir -p build-debug
cd build-debug
cmake -DCMAKE_BUILD_TYPE=Debug ../
make -j$(nproc)

# Create packages
cpack --config CPackConfig.cmake
```

#### Source Package
```bash
cd build
cpack --config CPackSourceConfig.cmake
```

## Package Installation

### Installing Release Packages
```bash
sudo dpkg -i build/libaasdk_*.deb build/libaasdk-dev_*.deb
```

### Installing Debug Packages
```bash
sudo dpkg -i build-debug/libaasdk-dbg_*.deb build-debug/libaasdk-dbg-dev_*.deb
```

### Installing from Repository (future)
```bash
# Add repository (when available)
sudo apt update
sudo apt install libaasdk libaasdk-dev
```

## Package Dependencies

The packages automatically handle the following dependencies:
- libusb-1.0-0 (>= 1.0.0)
- libboost-system1.81.0 | libboost-system1.74.0 (compatible with both versions)
- libboost-log1.81.0 | libboost-log1.74.0 (compatible with both versions)
- libssl3 (>= 3.0.0)
- libprotobuf32 (>= 3.21.0)

## Package Contents

### Runtime Package (libaasdk/libaasdk-dbg)
- `/usr/local/lib/libaasdk.so*` - Shared library files

### Development Package (libaasdk-dev/libaasdk-dbg-dev)
- `/usr/local/include/aasdk/` - Header files
- `/usr/local/lib/cmake/aasdk/` - CMake configuration files
- Depends on the corresponding runtime package

## Uninstalling

```bash
# Remove development package first
sudo dpkg -r libaasdk-dev

# Remove runtime package
sudo dpkg -r libaasdk

# Or for debug packages
sudo dpkg -r libaasdk-dbg-dev libaasdk-dbg
```

## Architecture Support

The build system supports:
- amd64 (x86_64)
- armhf (ARM 32-bit)
- Automatically detects architecture or uses TARGET_ARCH environment variable

## Cross-Compilation

For cross-compilation, set the TARGET_ARCH environment variable:

```bash
export TARGET_ARCH=armhf
cmake -DCMAKE_BUILD_TYPE=Release ../
```

## Troubleshooting

### Missing Dependencies
If you encounter dependency issues, install them manually:
```bash
sudo apt update
sudo apt install libusb-1.0-0-dev libboost-all-dev libssl-dev libprotobuf-dev protobuf-compiler
```

### Package Conflicts
If you have conflicting packages installed:
```bash
# List installed aasdk packages
dpkg -l | grep aasdk

# Remove conflicting packages
sudo dpkg -r package-name
```
