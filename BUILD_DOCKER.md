# AASDK Docker Build System

This directory contains the containerized build system for AASDK that supports multiple architectures using Docker and the native `build.sh` script.

## Quick Start

### Using the Docker Build Script (Recommended)

```bash
# Build for your current architecture
./docker-build.sh

# Build for specific architecture
./docker-build.sh arm64

# Build for all architectures
./docker-build.sh all

# Clean build
./docker-build.sh amd64 --clean
```

### Using Docker Compose

```bash
# Build for specific architecture
docker-compose up aasdk-amd64
docker-compose up aasdk-arm64
docker-compose up aasdk-armhf

# Build all architectures
docker-compose up build-all
```

### Using Docker directly

```bash
# Build the image
docker buildx build --platform linux/amd64 --build-arg TARGET_ARCH=amd64 -t aasdk-build:amd64 .

# Extract packages
docker create --name temp aasdk-build:amd64
docker cp temp:/output/. ./build-output/
docker rm temp
```

## Supported Architectures

- **amd64** (x86_64) - Native compilation
- **arm64** (aarch64) - Cross-compilation with emulation
- **armhf** (armv7) - Cross-compilation with emulation

## Requirements

- Docker with Buildx support
- QEMU for cross-platform emulation (automatically installed by Docker setup)

## Output

Built packages are placed in the `build-output/` directory:
- `*.deb` - Debian packages
- `*.tar.*` - Source archives

## Build Process

1. **Base Image**: Uses Debian Trixie for consistent dependencies
2. **Dependencies**: Installs native and cross-compilation toolchains
3. **Multi-arch**: Supports cross-compilation for ARM architectures
4. **Native Script**: Uses the existing `build.sh` script internally
5. **Packaging**: Automatically creates DEB packages using CPack

## Architecture-Specific Notes

### AMD64
- Native compilation on x86_64 hosts
- Fastest build time
- Full dependency resolution

### ARM64
- Cross-compilation with aarch64-linux-gnu toolchain
- Emulated execution using QEMU
- Native ARM64 libraries installed

### ARMHF
- Cross-compilation with arm-linux-gnueabihf toolchain
- Emulated execution using QEMU
- Native ARMHF libraries installed

## Troubleshooting

### Build Fails
1. Check Docker and Buildx installation
2. Ensure sufficient disk space
3. Clean build cache: `docker builder prune`

### Missing Packages
1. Check the `build-output/` directory
2. Look for build errors in Docker output
3. Verify the `build.sh` script runs correctly

### Cross-compilation Issues
1. Ensure QEMU emulation is working: `docker run --rm --privileged multiarch/qemu-user-static --reset -p yes`
2. Check that cross-compilation toolchains are installed
3. Verify target architecture libraries are available

## GitHub Actions Integration

The build system is integrated with GitHub Actions for automated builds:
- Triggers on push to main branches
- Builds all architectures in parallel
- Uploads artifacts for download
- Creates releases on git tags

## Local Development

For local development without Docker:
1. Install dependencies: `sudo apt install build-essential cmake libboost-all-dev libprotobuf-dev`
2. Use the native build script: `./build.sh release package`
3. Cross-compilation requires additional toolchains

## Performance Tips

- Use `--no-cache` sparingly as it significantly increases build time
- For development, build only the target architecture you need
- The Docker build cache speeds up subsequent builds significantly
- Multi-core systems benefit from parallel builds (automatically detected)

## Integration with OpenAuto

This AASDK build system is designed to integrate seamlessly with the OpenAuto project:
- Packages can be installed directly on target systems
- Headers and CMake files are included for development
- Compatible with existing OpenAuto build processes