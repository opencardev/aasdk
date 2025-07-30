# AASDK Troubleshooting Guide

This comprehensive troubleshooting guide helps diagnose and resolve common issues when building, deploying, and running AASDK applications.

## Table of Contents

1. [Quick Diagnostic Steps](#quick-diagnostic-steps)
2. [Build Issues](#build-issues)
3. [Runtime Issues](#runtime-issues)
4. [Performance Problems](#performance-problems)
5. [Platform-Specific Issues](#platform-specific-issues)
6. [DevContainer Issues](#devcontainer-issues)
7. [Network and Connectivity](#network-and-connectivity)
8. [Debugging Tools and Techniques](#debugging-tools-and-techniques)

---

## Quick Diagnostic Steps

### First Steps for Any Issue

1. **Check Build Status:**
   ```bash
   cd build-debug
   make clean && make -j$(nproc)
   ```

2. **Verify Dependencies:**
   ```bash
   ldd build-debug/lib/libaasdk.so
   pkg-config --modversion protobuf
   ```

3. **Check Logs:**
   ```bash
   # Enable debug logging
   export AASDK_LOG_LEVEL=DEBUG
   ./your_app 2>&1 | tee debug.log
   ```

4. **Test Basic Functionality:**
   ```bash
   cd build-debug
   ctest --output-on-failure
   ```

### Environment Check Script

Create a diagnostic script:
```bash
#!/bin/bash
# aasdk_diagnostic.sh

echo "🔍 AASDK Diagnostic Report"
echo "=========================="
echo

echo "📦 System Information:"
echo "OS: $(uname -a)"
echo "Architecture: $(uname -m)"
echo "Kernel: $(uname -r)"
echo

echo "🛠️ Build Tools:"
echo "GCC: $(gcc --version | head -1)"
echo "CMake: $(cmake --version | head -1)"
echo "Make: $(make --version | head -1)"
echo

echo "📚 Dependencies:"
echo "Boost: $(dpkg -l | grep libboost-dev || echo 'Not found')"
echo "USB: $(dpkg -l | grep libusb-1.0-0-dev || echo 'Not found')"
echo "SSL: $(dpkg -l | grep libssl-dev || echo 'Not found')"
echo "Protobuf: $(protoc --version || echo 'Not found')"
echo

echo "🔗 Library Paths:"
echo "LD_LIBRARY_PATH: $LD_LIBRARY_PATH"
echo "PKG_CONFIG_PATH: $PKG_CONFIG_PATH"
echo

echo "🏗️ Build Status:"
if [ -d "build-debug" ]; then
    echo "Debug build: ✅ Exists"
    ls -la build-debug/lib/ 2>/dev/null || echo "No libraries found"
else
    echo "Debug build: ❌ Missing"
fi

if [ -d "build-release" ]; then
    echo "Release build: ✅ Exists"
else
    echo "Release build: ❌ Missing"
fi
```

---

## Build Issues

### CMake Configuration Problems

#### Issue: "Could not find Boost"
```
CMake Error at /usr/share/cmake-3.x/Modules/FindBoost.cmake:
  Unable to find the requested Boost libraries
```

**Solution:**
```bash
# Ubuntu/Debian
sudo apt update
sudo apt install libboost-all-dev

# Check installation
dpkg -l | grep boost
ls /usr/lib/x86_64-linux-gnu/ | grep boost

# If still failing, try explicit paths
cmake -DBoost_ROOT=/usr \
      -DBoost_LIBRARY_DIRS=/usr/lib/x86_64-linux-gnu \
      -DBoost_INCLUDE_DIRS=/usr/include \
      ..
```

#### Issue: "Protobuf version mismatch"
```
error: 'GOOGLE_PROTOBUF_VERSION' was not declared
```

**Solutions:**

**Option 1 - System Package:**
```bash
sudo apt remove libprotobuf-dev protobuf-compiler
sudo apt install libprotobuf-dev protobuf-compiler

# Verify version compatibility
protoc --version  # Should be >= 3.21.0
```

**Option 2 - Build from Source:**
```bash
cd protobuf
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_INSTALL_PREFIX=/usr/local \
      -Dprotobuf_BUILD_TESTS=OFF \
      ..
make -j$(nproc)
sudo make install
sudo ldconfig
```

**Option 3 - Use Bundled Protobuf:**
```bash
# Build bundled protobuf first
cd aasdk
cmake -DBUILD_PROTOBUF=ON ..
make -j$(nproc) protobuf
make -j$(nproc)
```

#### Issue: "libusb.h: No such file or directory"
```
fatal error: libusb.h: No such file or directory
```

**Solution:**
```bash
# Install libusb development headers
sudo apt install libusb-1.0-0-dev

# Verify installation
pkg-config --cflags --libs libusb-1.0

# If pkg-config fails, install manually
sudo apt install pkg-config
```

### Cross-Compilation Issues

#### Issue: ARM Cross-Compiler Not Found
```
No CMAKE_C_COMPILER could be found.
CMAKE_C_COMPILER-NOTFOUND
```

**Solution:**
```bash
# Install ARM cross-compilation toolchain
sudo apt update
sudo apt install gcc-arm-linux-gnueabihf g++-arm-linux-gnueabihf
sudo apt install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu

# Verify installation
arm-linux-gnueabihf-gcc --version
aarch64-linux-gnu-gcc --version

# Set explicit compiler paths if needed
cmake -DCMAKE_C_COMPILER=/usr/bin/arm-linux-gnueabihf-gcc \
      -DCMAKE_CXX_COMPILER=/usr/bin/arm-linux-gnueabihf-g++ \
      -DTARGET_ARCH=armhf \
      ..
```

#### Issue: Cross-Compilation Library Paths
```
cannot find -lboost_system
```

**Solution:**
```bash
# Install ARM libraries
sudo apt install libboost-all-dev:armhf libusb-1.0-0-dev:armhf

# Or use multiarch
sudo dpkg --add-architecture armhf
sudo apt update
sudo apt install libboost-all-dev:armhf

# Set library search paths
export CMAKE_FIND_ROOT_PATH=/usr/arm-linux-gnueabihf
export CMAKE_FIND_ROOT_PATH_MODE_LIBRARY=ONLY
```

### Linker Errors

#### Issue: Undefined References
```
undefined reference to `boost::log::core::get()'
```

**Solution:**
```bash
# Check if boost log is properly linked
ldd build-debug/lib/libaasdk.so | grep boost

# Add explicit linking
cmake -DCMAKE_EXE_LINKER_FLAGS="-lboost_system -lboost_log" ..

# Or use find_package with components
find_package(Boost REQUIRED COMPONENTS system log thread)
target_link_libraries(aasdk ${Boost_LIBRARIES})
```

---

## Runtime Issues

### USB Permission Problems

#### Issue: "Permission denied" for USB device
```
libusb: error [get_usbfs_fd] libusb couldn't open USB device /dev/bus/usb/001/002: Permission denied
```

**Solutions:**

**Option 1 - udev Rules:**
```bash
# Create udev rule for Android devices
sudo tee /etc/udev/rules.d/51-android.rules << 'EOF'
# Android devices
SUBSYSTEM=="usb", ATTR{idVendor}=="18d1", MODE="0666", GROUP="plugdev"
SUBSYSTEM=="usb", ATTR{idVendor}=="22b8", MODE="0666", GROUP="plugdev"
SUBSYSTEM=="usb", ATTR{idVendor}=="04e8", MODE="0666", GROUP="plugdev"
SUBSYSTEM=="usb", ATTR{idVendor}=="0bb4", MODE="0666", GROUP="plugdev"

# Google devices
SUBSYSTEM=="usb", ATTR{idVendor}=="18d1", ATTR{idProduct}=="4ee7", MODE="0666", GROUP="plugdev"
SUBSYSTEM=="usb", ATTR{idVendor}=="18d1", ATTR{idProduct}=="4ee1", MODE="0666", GROUP="plugdev"
EOF

# Reload udev rules
sudo udevadm control --reload-rules
sudo udevadm trigger

# Add user to plugdev group
sudo usermod -a -G plugdev $USER
newgrp plugdev  # or logout/login
```

**Option 2 - Run as Root (NOT recommended for production):**
```bash
sudo ./your_aasdk_app
```

**Option 3 - Temporary Permissions:**
```bash
# Find the device
lsusb
# Set temporary permissions
sudo chmod 666 /dev/bus/usb/001/002  # Replace with your device
```

### SSL/TLS Certificate Issues

#### Issue: SSL handshake failures
```
SSL_connect failed: certificate verify failed
```

**Solutions:**

**Option 1 - Update CA Certificates:**
```bash
sudo apt update
sudo apt install ca-certificates
sudo update-ca-certificates
```

**Option 2 - SSL Context Configuration:**
```cpp
// In your code, configure SSL context properly
SSL_CTX* ctx = SSL_CTX_new(TLS_client_method());
SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, nullptr);
SSL_CTX_set_default_verify_paths(ctx);

// For development only - disable verification
SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, nullptr);
```

**Option 3 - OpenSSL Configuration:**
```bash
# Check OpenSSL configuration
openssl version -a
openssl s_client -connect your_target:443 -verify_return_error

# Update OpenSSL if needed
sudo apt install openssl libssl-dev
```

### Audio System Issues

#### Issue: "ALSA lib pcm_dmix.c: unable to open slave"
```
ALSA lib pcm_dmix.c:1108:(snd_pcm_dmix_open) unable to open slave
```

**Solutions:**

**Option 1 - Install PulseAudio:**
```bash
sudo apt install pulseaudio pulseaudio-utils alsa-utils
systemctl --user enable pulseaudio
systemctl --user start pulseaudio
```

**Option 2 - Configure ALSA:**
```bash
# Check available audio devices
aplay -l
cat /proc/asound/cards

# Test audio output
speaker-test -t wav -c 2

# Create/edit ~/.asoundrc
cat > ~/.asoundrc << 'EOF'
pcm.!default {
    type pulse
}
ctl.!default {
    type pulse
}
EOF
```

**Option 3 - Check Audio Permissions:**
```bash
# Add user to audio group
sudo usermod -a -G audio $USER

# Check audio device permissions
ls -la /dev/snd/
```

### Video Display Issues

#### Issue: No video output/display
```
[ERROR][VIDEO] Failed to initialize display context
```

**Solutions:**

**Option 1 - X11 Display:**
```bash
# Check display environment
echo $DISPLAY
xrandr  # List available displays

# Set display if needed
export DISPLAY=:0
```

**Option 2 - Wayland Compatibility:**
```bash
# For Wayland sessions
export WAYLAND_DISPLAY=wayland-0
export XDG_RUNTIME_DIR=/run/user/$(id -u)
```

**Option 3 - Graphics Drivers:**
```bash
# Check graphics drivers
lspci | grep VGA
glxinfo | grep OpenGL

# Install appropriate drivers
sudo apt install mesa-utils
```

### Memory Issues

#### Issue: Memory leaks detected
```
==123== LEAK SUMMARY:
==123==    definitely lost: 1,024 bytes in 1 blocks
```

**Debugging:**
```bash
# Build with AddressSanitizer
cmake -DCMAKE_CXX_FLAGS="-fsanitize=address -g" ..
make -j$(nproc)

# Run with detailed output
ASAN_OPTIONS="verbosity=1:abort_on_error=1" ./your_app

# Use Valgrind for detailed analysis
valgrind --tool=memcheck \
         --leak-check=full \
         --show-leak-kinds=all \
         --track-origins=yes \
         --log-file=memcheck.log \
         ./your_app
```

---

## Performance Problems

### High CPU Usage

#### Issue: Excessive CPU consumption
```
top shows 100% CPU usage for AASDK application
```

**Diagnosis:**
```bash
# Profile with perf
perf record -g ./your_app
perf report

# Check thread activity
htop -H -p $(pgrep your_app)

# Monitor system calls
strace -p $(pgrep your_app) -f -e trace=all
```

**Solutions:**

**Option 1 - Optimize Logging:**
```cpp
// Reduce logging verbosity
aasdk::common::ModernLogger::setLevel(aasdk::common::LogLevel::WARN);

// Use async logging
aasdk::common::ModernLogger::enableAsync(true);
```

**Option 2 - Thread Optimization:**
```cpp
// Set appropriate thread priorities
#include <pthread.h>

void setThreadPriority(pthread_t thread, int priority) {
    struct sched_param param;
    param.sched_priority = priority;
    pthread_setschedparam(thread, SCHED_FIFO, &param);
}
```

**Option 3 - Buffer Size Optimization:**
```cpp
// Optimize audio buffer sizes
const size_t OPTIMAL_BUFFER_SIZE = 1024; // Adjust based on testing
audioChannel->setBufferSize(OPTIMAL_BUFFER_SIZE);
```

### Memory Usage Issues

#### Issue: Excessive memory consumption
```
Process using > 500MB RAM
```

**Diagnosis:**
```bash
# Monitor memory usage
watch -n 1 'ps aux | grep your_app | grep -v grep'

# Detailed memory analysis
pmap $(pgrep your_app)
cat /proc/$(pgrep your_app)/status | grep -i mem

# Check for memory leaks
valgrind --tool=massif ./your_app
ms_print massif.out.12345
```

**Solutions:**

**Option 1 - Buffer Management:**
```cpp
// Use memory pools for frequent allocations
class BufferPool {
    std::vector<std::unique_ptr<Buffer>> pool_;
public:
    std::unique_ptr<Buffer> acquire() {
        if (!pool_.empty()) {
            auto buffer = std::move(pool_.back());
            pool_.pop_back();
            return buffer;
        }
        return std::make_unique<Buffer>();
    }
    
    void release(std::unique_ptr<Buffer> buffer) {
        pool_.push_back(std::move(buffer));
    }
};
```

**Option 2 - Smart Pointer Optimization:**
```cpp
// Use appropriate smart pointer types
std::shared_ptr<Data> data = std::make_shared<Data>(); // For shared ownership
std::unique_ptr<Handler> handler = std::make_unique<Handler>(); // For exclusive ownership

// Avoid circular references with weak_ptr
std::weak_ptr<Parent> parent_ref = parent;
```

### Network Performance Issues

#### Issue: Slow TCP throughput
```
TCP transfer rate < 10 MB/s on Gigabit network
```

**Diagnosis:**
```bash
# Test network bandwidth
iperf3 -c target_ip -p 5277

# Monitor network traffic
sudo netstat -i
sudo iftop -i eth0

# Check TCP settings
cat /proc/sys/net/ipv4/tcp_window_scaling
cat /proc/sys/net/core/rmem_max
```

**Solutions:**

**Option 1 - TCP Buffer Tuning:**
```cpp
// Set socket buffer sizes
int sock_buf_size = 1024 * 1024; // 1MB
setsockopt(socket_fd, SOL_SOCKET, SO_RCVBUF, &sock_buf_size, sizeof(sock_buf_size));
setsockopt(socket_fd, SOL_SOCKET, SO_SNDBUF, &sock_buf_size, sizeof(sock_buf_size));

// Disable Nagle's algorithm for low latency
int flag = 1;
setsockopt(socket_fd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));
```

**Option 2 - System-Level Tuning:**
```bash
# Increase network buffer sizes
echo 'net.core.rmem_max = 16777216' | sudo tee -a /etc/sysctl.conf
echo 'net.core.wmem_max = 16777216' | sudo tee -a /etc/sysctl.conf
sudo sysctl -p
```

---

## Platform-Specific Issues

### Raspberry Pi Issues

#### Issue: ARM compilation failures
```
Internal compiler error on ARM
```

**Solutions:**

**Option 1 - Reduce Optimization:**
```bash
cmake -DCMAKE_CXX_FLAGS="-O1 -g" -DTARGET_ARCH=armhf ..
```

**Option 2 - Increase Memory:**
```bash
# Increase swap space
sudo dphys-swapfile swapoff
sudo vim /etc/dphys-swapfile
# Set CONF_SWAPSIZE=2048
sudo dphys-swapfile setup
sudo dphys-swapfile swapon
```

**Option 3 - Use Cross-Compilation:**
```bash
# Cross-compile on x64 host instead of native ARM compilation
cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/arm-linux-gnueabihf.cmake ..
```

#### Issue: USB hotplug not working on Pi
```
USB device insertion not detected
```

**Solution:**
```bash
# Install udev and configure properly
sudo apt install udev

# Create Pi-specific udev rule
sudo tee /etc/udev/rules.d/99-android-pi.rules << 'EOF'
SUBSYSTEM=="usb", ACTION=="add", ATTRS{idVendor}=="18d1", RUN+="/bin/chmod 666 %E{DEVNAME}"
SUBSYSTEM=="usb", ACTION=="add", ATTRS{idVendor}=="18d1", RUN+="/usr/bin/logger 'Android device connected'"
EOF

sudo systemctl reload udev
```

### Windows WSL Issues

#### Issue: USB access in WSL
```
USB devices not accessible in WSL environment
```

**Solutions:**

**Option 1 - Use WSL2 with USB/IP:**
```bash
# Install usbipd on Windows host
winget install usbipd

# In WSL2, install tools
sudo apt install linux-tools-virtual hwdata
sudo update-alternatives --install /usr/local/bin/usbip usbip `ls /usr/lib/linux-tools/*/usbip | tail -n1` 20

# Attach USB device from Windows
# (Run in Windows PowerShell as Administrator)
usbipd wsl list
usbipd wsl attach --busid 1-1
```

**Option 2 - Use TCP Transport Instead:**
```cpp
// Configure for wireless Android Auto
auto transport = std::make_unique<aasdk::transport::TCPTransport>();
transport->bind("0.0.0.0", 5277);
```

### macOS Issues

#### Issue: Dependencies installation on macOS
```
brew install failures for dependencies
```

**Solution:**
```bash
# Use Homebrew for dependencies
brew update
brew install cmake boost libusb protobuf openssl

# Set proper paths
export PKG_CONFIG_PATH="/usr/local/opt/openssl/lib/pkgconfig:$PKG_CONFIG_PATH"
export LDFLAGS="-L/usr/local/opt/openssl/lib"
export CPPFLAGS="-I/usr/local/opt/openssl/include"

# For Apple Silicon Macs
export PATH="/opt/homebrew/bin:$PATH"
```

---

## DevContainer Issues

### Container Build Failures

#### Issue: DevContainer fails to build
```
ERROR: failed to solve: process "/bin/sh -c apt-get update" did not complete successfully
```

**Solutions:**

**Option 1 - Clear Docker Cache:**
```bash
docker system prune -a
docker builder prune
```

**Option 2 - Rebuild Container:**
```bash
# In VS Code: Ctrl+Shift+P
# "Dev Containers: Rebuild Container"
```

**Option 3 - Check Docker Resources:**
```bash
# Increase Docker memory/disk limits in Docker Desktop settings
# Memory: 8GB+
# Disk: 64GB+
```

### Mount Issues

#### Issue: Volume mount permissions
```
Permission denied when accessing files in container
```

**Solutions:**

**Option 1 - Fix Ownership:**
```bash
# In container
sudo chown -R vscode:vscode /workspace
```

**Option 2 - Update devcontainer.json:**
```json
{
    "mounts": [
        "source=${localWorkspaceFolder},target=/workspace,type=bind,consistency=cached"
    ],
    "containerUser": "vscode",
    "updateRemoteUserUID": true
}
```

### Architecture Selection Issues

#### Issue: Wrong architecture container selected
```
ARM binary won't run on x64 system
```

**Solution:**
```bash
# Verify current architecture
uname -m
docker info | grep Architecture

# Select correct devcontainer
# Ctrl+Shift+P → "Dev Containers: Reopen in Container"
# Choose the appropriate .devcontainer-xxx.json file
```

---

## Network and Connectivity

### WiFi AndroidAuto Issues

#### Issue: Device not connecting wirelessly
```
[ERROR][TCP] No client connections after 30 seconds
```

**Diagnosis:**
```bash
# Check network connectivity
ping phone_ip_address
telnet phone_ip_address 5277

# Check firewall
sudo ufw status
sudo iptables -L

# Monitor network traffic
sudo tcpdump -i wlan0 port 5277
```

**Solutions:**

**Option 1 - Firewall Configuration:**
```bash
# Allow AndroidAuto port
sudo ufw allow 5277
sudo firewall-cmd --add-port=5277/tcp --permanent
```

**Option 2 - Network Interface Selection:**
```cpp
// Bind to specific interface
auto transport = std::make_unique<aasdk::transport::TCPTransport>();
transport->bind("192.168.1.100", 5277); // Use specific IP instead of 0.0.0.0
```

**Option 3 - mDNS Configuration:**
```bash
# Install and configure Avahi for device discovery
sudo apt install avahi-daemon avahi-utils
sudo systemctl enable avahi-daemon
sudo systemctl start avahi-daemon
```

### Bluetooth Issues

#### Issue: Bluetooth channel not working
```
[ERROR][BLUETOOTH] Failed to initialize Bluetooth adapter
```

**Solutions:**

**Option 1 - BlueZ Setup:**
```bash
# Install BlueZ stack
sudo apt install bluez bluez-tools

# Enable Bluetooth service
sudo systemctl enable bluetooth
sudo systemctl start bluetooth

# Check Bluetooth status
bluetoothctl show
```

**Option 2 - Permissions:**
```bash
# Add user to bluetooth group
sudo usermod -a -G bluetooth $USER
```

**Option 3 - D-Bus Configuration:**
```bash
# Check D-Bus Bluetooth access
dbus-send --system --print-reply --dest=org.bluez / org.freedesktop.DBus.Introspectable.Introspect
```

---

## Debugging Tools and Techniques

### GDB Debugging

**Basic Debugging Session:**
```bash
# Build with debug symbols
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j$(nproc)

# Start GDB session
gdb ./your_aasdk_app

# Useful GDB commands
(gdb) set logging on
(gdb) set logging file debug.log
(gdb) run
(gdb) bt           # Backtrace when crashed
(gdb) info threads # Show all threads
(gdb) thread 2     # Switch to thread 2
(gdb) print variable_name
(gdb) watch variable_name  # Watchpoint
```

**Multi-threaded Debugging:**
```bash
# GDB for multi-threaded applications
(gdb) set scheduler-locking step
(gdb) thread apply all bt
(gdb) info threads
```

### Logging and Tracing

**Enable Comprehensive Logging:**
```cpp
// At application startup
aasdk::common::ModernLogger::initialize();
aasdk::common::ModernLogger::setLevel(aasdk::common::LogLevel::TRACE);

// Enable all categories
aasdk::common::ModernLogger::setLevel(aasdk::common::LogCategory::TRANSPORT, aasdk::common::LogLevel::DEBUG);
aasdk::common::ModernLogger::setLevel(aasdk::common::LogCategory::USB, aasdk::common::LogLevel::DEBUG);
aasdk::common::ModernLogger::setLevel(aasdk::common::LogCategory::TCP, aasdk::common::LogLevel::DEBUG);
```

**System Call Tracing:**
```bash
# Trace system calls
strace -f -o strace.log ./your_app

# Trace specific calls
strace -e trace=open,read,write,close ./your_app

# Trace network calls
strace -e trace=network ./your_app
```

### Performance Profiling

**CPU Profiling with perf:**
```bash
# Record performance data
perf record -g --call-graph dwarf ./your_app

# Analyze hotspots
perf report
perf top

# Generate flame graph
perf script | stackcollapse-perf.pl | flamegraph.pl > profile.svg
```

**Memory Profiling:**
```bash
# Heap profiling with Valgrind
valgrind --tool=massif --pages-as-heap=yes ./your_app

# Analysis
ms_print massif.out.12345

# Real-time memory monitoring
watch -n 1 'cat /proc/$(pgrep your_app)/status | grep -E "(VmRSS|VmSize)"'
```

### Protocol Analysis

**AndroidAuto Protocol Debugging:**
```bash
# Capture USB traffic (requires special hardware)
# Use USB protocol analyzer or software-based capture

# Capture TCP traffic for wireless AndroidAuto
tcpdump -i any -w androidauto.pcap 'port 5277'

# Analyze with Wireshark
wireshark androidauto.pcap

# Or use tshark for command-line analysis
tshark -r androidauto.pcap -Y "tcp.port == 5277" -T fields -e frame.time -e tcp.payload
```

---

## Recovery Procedures

### Factory Reset/Clean Build

**Complete Clean Build:**
```bash
#!/bin/bash
# clean_build.sh

echo "🧹 Performing complete clean build..."

# Remove all build directories
rm -rf build-debug build-release

# Clean protobuf
cd protobuf && rm -rf build && cd ..

# Remove any cached CMake files
find . -name "CMakeCache.txt" -delete
find . -name "CMakeFiles" -type d -exec rm -rf {} +

# Rebuild protobuf
mkdir -p protobuf/build
cd protobuf/build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
make install
cd ../..

# Fresh debug build
mkdir -p build-debug
cd build-debug
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j$(nproc)
cd ..

echo "✅ Clean build completed!"
```

### Emergency Debug Mode

**Create Emergency Debug Build:**
```bash
# Emergency build with maximum debugging
cmake -DCMAKE_BUILD_TYPE=Debug \
      -DCMAKE_CXX_FLAGS="-g3 -O0 -fsanitize=address -fsanitize=undefined" \
      -DENABLE_COVERAGE=ON \
      ..
```

### Contact Support

When all else fails, gather this information for support:

1. **System Information:**
   ```bash
   ./aasdk_diagnostic.sh > diagnostic_report.txt
   ```

2. **Build Logs:**
   ```bash
   make clean && make -j$(nproc) 2>&1 | tee build.log
   ```

3. **Runtime Logs:**
   ```bash
   AASDK_LOG_LEVEL=TRACE ./your_app 2>&1 | tee runtime.log
   ```

4. **Test Results:**
   ```bash
   ctest --output-on-failure 2>&1 | tee test.log
   ```

Submit these files along with:
- Description of the issue
- Steps to reproduce
- Expected vs actual behavior
- Your specific use case

---

*This troubleshooting guide covers the most common issues. For additional help, check the GitHub issues or community discussions.*
