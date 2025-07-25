# AASDK Multi-Architecture Development Environment

This project now includes a comprehensive development environment using VS Code devcontainers that supports building AASDK for multiple architectures: **x64**, **ARM64**, and **ARMHF**.

## 🚀 Quick Start

### Prerequisites
- [Docker Desktop](https://www.docker.com/products/docker-desktop/) installed and running
- [VS Code](https://code.visualstudio.com/) with [Dev Containers extension](https://marketplace.visualstudio.com/items?itemName=ms-vscode-remote.remote-containers)

### Launch Development Environment

1. **Open VS Code in the project directory**
2. **Choose your target architecture**:

   | Architecture | Use Case | Command |
   |-------------|----------|---------|
   | **x64 (Default)** | Native development, fastest builds | `Ctrl+Shift+P` → "Dev Containers: Reopen in Container" |
   | **ARM64** | Raspberry Pi 4, Apple Silicon targets | `Ctrl+Shift+P` → "Dev Containers: Reopen in Container" → Select "devcontainer-arm64.json" |
   | **ARMHF** | Raspberry Pi 3, older ARM devices | `Ctrl+Shift+P` → "Dev Containers: Reopen in Container" → Select "devcontainer-armhf.json" |

3. **Wait for container setup** (first time takes ~5-10 minutes)
4. **Start building!**

## 🔨 Building the Project

### Option 1: Quick Build Scripts (Recommended)
```bash
# Build debug version
./build.sh debug

# Build release version
./build.sh release

# Clean and rebuild
./build.sh debug clean
```

### Option 2: VS Code Integrated Tasks
- **Press `Ctrl+Shift+P`** → "Tasks: Run Task"
- Choose from:
  - `DevContainer: Build Debug (Quick)`
  - `DevContainer: Build Release (Quick)`
  - `DevContainer: Clean & Build`
  - `DevContainer: Create Packages`

### Option 3: CMake Tools Extension
- Use the CMake extension's **Build** button in the status bar
- Configure with `Ctrl+Shift+P` → "CMake: Configure"
- Build with `Ctrl+Shift+P` → "CMake: Build"

## 📦 Creating Packages

```bash
# Create release packages (after building)
./package.sh release

# Create debug packages  
./package.sh debug
```

Packages will be created in the `packages/` directory.

## 🐛 Debugging

### Integrated Debugging
1. Set breakpoints in your code
2. Press **F5** or use the Debug panel
3. Choose from available configurations:
   - `Debug AASDK Application`
   - `Debug Unit Tests`
   - `Attach to Process`

### Manual GDB Debugging
```bash
cd build-debug
gdb ./your-executable
```

## 🏗️ Architecture Details

### Container Specifications

| Architecture | Platform | Compiler | Cross-Compilation |
|-------------|----------|----------|-------------------|
| **x64** | `linux/amd64` | GCC/G++ (native) | ❌ Native build |
| **ARM64** | `linux/arm64` | GCC/G++ (native) | ❌ Native build (emulated on x64) |
| **ARMHF** | `linux/arm/v7` | `arm-linux-gnueabihf-gcc` | ✅ Cross-compilation |

### Environment Variables

Each container automatically sets:
- `TARGET_ARCH`: Architecture identifier (`amd64`, `arm64`, `armhf`)
- `CC`/`CXX`: Appropriate compilers for cross-compilation

### Build Directories

```
build-debug/     # Debug builds
build-release/   # Release builds  
packages/        # Generated packages
protobuf/build/  # Protobuf library builds
```

## 📁 Project Structure

```
.devcontainer/
├── devcontainer.json           # Default x64 configuration
├── devcontainer-arm64.json     # ARM64 configuration
├── devcontainer-armhf.json     # ARMHF configuration
├── docker-compose.yml          # Multi-service container setup
├── Dockerfile.x64              # x64 container definition
├── Dockerfile.arm64            # ARM64 container definition
├── Dockerfile.armhf            # ARMHF container definition
├── post-create.sh              # Setup script
└── README.md                   # Detailed documentation

.vscode/
├── tasks.json                  # Enhanced build tasks
├── launch.json                 # Debug configurations  
└── settings.json               # Project settings

CMakePresets.json               # CMake presets for all architectures
```

## 🛠️ Included Development Tools

### VS Code Extensions
- **C/C++ Extension Pack**: IntelliSense, debugging, formatting
- **CMake Tools**: Integrated CMake support
- **Additional tools**: JSON/YAML support, hex editor

### Build Tools & Libraries
- CMake 3.14+
- GCC/G++ toolchains (including cross-compilers)
- Protocol Buffers compiler & libraries
- Boost libraries (system, filesystem, thread, log, etc.)
- libusb & OpenSSL development libraries
- Git & GitHub CLI

## ⚡ Performance Notes

| Architecture | Build Speed | Notes |
|-------------|-------------|-------|
| **x64** | ⚡ Fastest | Native execution |
| **ARM64** | 🔶 Good | Native on Apple Silicon, emulated on x64 |
| **ARMHF** | 🐌 Slower | Always emulated/cross-compiled |

## 🔧 Advanced Usage

### Manual Docker Compose
```bash
# Start specific architecture container
docker-compose -f .devcontainer/docker-compose.yml up aasdk-dev        # x64
docker-compose -f .devcontainer/docker-compose.yml up aasdk-dev-arm64  # ARM64
docker-compose -f .devcontainer/docker-compose.yml up aasdk-dev-armhf  # ARMHF
```

### Custom CMake Configuration
```bash
# Manual CMake build with specific architecture
mkdir -p build-custom
cd build-custom
cmake -DCMAKE_BUILD_TYPE=Release -DTARGET_ARCH=arm64 ..
make -j$(nproc)
```

### Container Customization
Edit the appropriate `Dockerfile.*` to add additional dependencies or tools for your specific architecture.

## 🐛 Troubleshooting

### Container Won't Start
```bash
# Check Docker status
docker --version
docker info

# For ARM containers on x64, ensure buildx is enabled
docker buildx ls
```

### Build Issues
```bash
# Check environment variables
echo $TARGET_ARCH

# Verify protobuf is built
ls -la protobuf/build/

# Check compiler availability
which gcc g++ # For x64
which arm-linux-gnueabihf-gcc # For ARMHF
which aarch64-linux-gnu-gcc   # For ARM64 cross-compilation
```

### Performance Issues
- Use **x64** container for fastest development iteration
- Build **ARM64/ARMHF** only when targeting specific devices
- Enable Docker's experimental features for better emulation

## 🎯 Development Workflow

1. **Start with x64** for rapid development and testing
2. **Switch to target architecture** for final builds and testing
3. **Use integrated debugging** for troubleshooting
4. **Create packages** for deployment

The devcontainer environment provides a consistent, reproducible development experience across all supported architectures while maintaining the flexibility to target specific embedded platforms.

## 📚 Additional Resources

- [Dev Containers Documentation](https://code.visualstudio.com/docs/devcontainers/containers)
- [Docker Multi-platform builds](https://docs.docker.com/desktop/multi-arch/)
- [CMake Cross-compilation](https://cmake.org/cmake/help/latest/manual/cmake-toolchains.7.html)
- [VS Code CMake Tools](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cmake-tools)
