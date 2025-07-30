# AASDK Testing Guide

This guide covers all aspects of testing the AASDK library, from unit tests to integration testing and performance validation.

## Table of Contents

1. [Test Types Overview](#test-types-overview)
2. [Unit Testing](#unit-testing)
3. [Integration Testing](#integration-testing)
4. [Performance Testing](#performance-testing)
5. [Cross-Platform Testing](#cross-platform-testing)
6. [Automated Testing](#automated-testing)
7. [Manual Testing Procedures](#manual-testing-procedures)
8. [Test Data and Fixtures](#test-data-and-fixtures)

---

## Test Types Overview

### Test Pyramid Structure

```
        ┌─────────────────────┐
        │   Manual Tests      │  ← Integration, End-to-End
        │                     │
        ├─────────────────────┤
        │ Integration Tests   │  ← Component Integration
        │                     │
        ├─────────────────────┤
        │   Unit Tests        │  ← Individual Components
        │                     │
        └─────────────────────┘
```

### Test Categories

| Type | Purpose | Tools | Frequency |
|------|---------|-------|-----------|
| **Unit Tests** | Test individual classes/functions | Google Test | Every commit |
| **Integration Tests** | Test component interactions | Custom test harness | Daily builds |
| **Performance Tests** | Measure performance metrics | Valgrind, perf | Weekly |
| **Memory Tests** | Check for leaks and corruption | AddressSanitizer, Valgrind | Every PR |
| **Threading Tests** | Detect race conditions | ThreadSanitizer | Every PR |
| **Platform Tests** | Multi-architecture validation | DevContainers | Release builds |

---

## Unit Testing

### Running Unit Tests

**Quick Test Run:**
```bash
cd build-debug
ctest --output-on-failure
```

**Verbose Test Output:**
```bash
cd build-debug
ctest --verbose
```

**Run Specific Test Suite:**
```bash
# Test specific components
./test_aasdk_transport
./test_aasdk_channel  
./test_aasdk_protocol
./test_aasdk_common
```

**Run Tests with Pattern:**
```bash
ctest -R "transport"  # Run all transport tests
ctest -R "usb|tcp"    # Run USB and TCP tests
```

### Test Coverage

**Generate Coverage Report:**
```bash
# Build with coverage enabled
cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_COVERAGE=ON ..
make -j$(nproc)

# Run tests
ctest

# Generate coverage report
make coverage

# View HTML report
firefox coverage/index.html
```

**Coverage Targets:**
- **Unit Tests:** > 90% line coverage
- **Integration Tests:** > 80% branch coverage
- **Critical Paths:** 100% coverage (security, error handling)

### Writing Unit Tests

**Test Structure Example:**
```cpp
#include <gtest/gtest.h>
#include <aasdk/Transport/USBTransport.hpp>
#include <aasdk/Common/Log.hpp>

class USBTransportTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize test environment
        aasdk::common::ModernLogger::initialize();
        transport_ = std::make_unique<aasdk::transport::USBTransport>();
    }
    
    void TearDown() override {
        // Cleanup
        transport_.reset();
    }
    
    std::unique_ptr<aasdk::transport::USBTransport> transport_;
};

TEST_F(USBTransportTest, InitializationSuccess) {
    EXPECT_NO_THROW(transport_->initialize());
    EXPECT_TRUE(transport_->isInitialized());
}

TEST_F(USBTransportTest, SendDataValidBuffer) {
    aasdk::common::DataBuffer buffer(1024);
    buffer.fill(0xAA);
    
    EXPECT_NO_THROW(transport_->send(buffer));
}

TEST_F(USBTransportTest, SendDataInvalidBuffer) {
    aasdk::common::DataBuffer emptyBuffer;
    
    EXPECT_THROW(transport_->send(emptyBuffer), std::invalid_argument);
}
```

**Mock Objects Example:**
```cpp
class MockTransport : public aasdk::transport::ITransport {
public:
    MOCK_METHOD(void, start, (), (override));
    MOCK_METHOD(void, stop, (), (override));
    MOCK_METHOD(void, send, (const aasdk::common::DataBuffer&), (override));
};

TEST(ChannelTest, SendsDataThroughTransport) {
    MockTransport mockTransport;
    auto channel = std::make_unique<aasdk::channel::AudioChannel>(&mockTransport);
    
    aasdk::common::DataBuffer testData(512);
    
    EXPECT_CALL(mockTransport, send(::testing::_))
        .Times(1);
    
    channel->sendAudioData(testData);
}
```

---

## Integration Testing

### Logger Integration Test

**Test Modern Logger:**
```bash
# Build test program
g++ -std=c++17 test_logger.cpp -o test_logger \
    -I./include \
    -L./build-debug/lib \
    -laasdk \
    -lboost_system \
    -lboost_log \
    -lpthread

# Run comprehensive logger test
./test_logger
```

**Expected Output:**
```
🧪 AASDK Modern Logger Test Suite
================================

✅ Basic Logging Test
✅ Category-Specific Logging Test  
✅ Structured Logging Test
✅ Performance Test (1000 messages in <100ms)
✅ Thread Safety Test
✅ File Rotation Test

📊 Test Summary: 6/6 tests passed
```

### Transport Integration Test

**USB Transport Test:**
```cpp
#include <aasdk/Transport/USBTransport.hpp>
#include <chrono>
#include <thread>

class USBIntegrationTest {
public:
    bool testUSBDeviceDetection() {
        auto transport = std::make_unique<aasdk::transport::USBTransport>();
        
        bool deviceDetected = false;
        transport->setDeviceHandler([&](const aasdk::usb::USBDevice& device) {
            deviceDetected = true;
            AASDK_LOG_USB_INFO() << "Device detected: " << device.getDescription();
        });
        
        transport->start();
        
        // Wait for device detection (timeout after 10 seconds)
        auto start = std::chrono::steady_clock::now();
        while (!deviceDetected && 
               std::chrono::steady_clock::now() - start < std::chrono::seconds(10)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        transport->stop();
        return deviceDetected;
    }
};
```

### Protocol Integration Test

**AndroidAuto Protocol Test:**
```cpp
class ProtocolIntegrationTest {
public:
    bool testHandshakeSequence() {
        // Test complete handshake with mock Android device
        auto transport = createMockTransport();
        auto protocol = std::make_unique<aasdk::protocol::AAProtocol>(transport);
        
        bool handshakeComplete = false;
        protocol->setHandshakeHandler([&](bool success) {
            handshakeComplete = success;
        });
        
        // Simulate Android device connection
        simulateAndroidConnection(transport);
        
        return handshakeComplete;
    }
};
```

---

## Performance Testing

### Memory Performance

**Memory Leak Detection:**
```bash
# Build with AddressSanitizer
cmake -DCMAKE_CXX_FLAGS="-fsanitize=address -g" ..
make -j$(nproc)

# Run application
./your_aasdk_app

# Check for leaks
echo "No leaks detected" || echo "Memory leaks found!"
```

**Valgrind Analysis:**
```bash
# Memory leak check
valgrind --tool=memcheck \
         --leak-check=full \
         --show-leak-kinds=all \
         --track-origins=yes \
         --log-file=valgrind_memcheck.log \
         ./your_aasdk_app

# Performance profiling
valgrind --tool=callgrind \
         --callgrind-out-file=callgrind.out \
         ./your_aasdk_app

# Analyze with kcachegrind
kcachegrind callgrind.out
```

### CPU Performance

**Profiling with perf:**
```bash
# Record performance data
perf record -g --call-graph dwarf ./your_aasdk_app

# Analyze performance
perf report

# Look for hotspots
perf top
```

**CPU Usage Monitoring:**
```bash
# Monitor real-time CPU usage
top -p $(pgrep your_aasdk_app)

# Detailed CPU analysis
htop -p $(pgrep your_aasdk_app)
```

### Network Performance

**TCP Transport Performance:**
```cpp
class NetworkPerformanceTest {
public:
    void testThroughput() {
        auto transport = std::make_unique<aasdk::transport::TCPTransport>();
        
        const size_t dataSize = 1024 * 1024; // 1MB
        const int iterations = 100;
        
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < iterations; ++i) {
            aasdk::common::DataBuffer buffer(dataSize);
            transport->send(buffer);
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        double throughputMBps = (dataSize * iterations / 1024.0 / 1024.0) / (duration.count() / 1000.0);
        
        AASDK_LOG_TRANSPORT_INFO() << "TCP Throughput: " << throughputMBps << " MB/s";
        
        // Assert minimum performance threshold
        ASSERT_GT(throughputMBps, 10.0); // Minimum 10 MB/s
    }
};
```

### Audio Performance

**Audio Latency Test:**
```cpp
class AudioPerformanceTest {
public:
    void testAudioLatency() {
        auto audioChannel = std::make_unique<aasdk::channel::AudioChannel>();
        
        std::vector<std::chrono::microseconds> latencies;
        
        for (int i = 0; i < 1000; ++i) {
            auto start = std::chrono::high_resolution_clock::now();
            
            // Simulate audio processing
            aasdk::audio::AudioData audioData(1024);
            audioChannel->processAudio(audioData);
            
            auto end = std::chrono::high_resolution_clock::now();
            latencies.push_back(std::chrono::duration_cast<std::chrono::microseconds>(end - start));
        }
        
        // Calculate statistics
        auto avgLatency = std::accumulate(latencies.begin(), latencies.end(), std::chrono::microseconds(0)) / latencies.size();
        auto maxLatency = *std::max_element(latencies.begin(), latencies.end());
        
        AASDK_LOG_AUDIO_MEDIA_INFO() << "Average audio latency: " << avgLatency.count() << " μs";
        AASDK_LOG_AUDIO_MEDIA_INFO() << "Maximum audio latency: " << maxLatency.count() << " μs";
        
        // Assert acceptable latency thresholds
        ASSERT_LT(avgLatency.count(), 1000); // < 1ms average
        ASSERT_LT(maxLatency.count(), 5000); // < 5ms maximum
    }
};
```

---

## Cross-Platform Testing

### Architecture Testing Matrix

| Architecture | Container | Test Focus |
|-------------|-----------|------------|
| **x64** | Default | Full test suite, performance baseline |
| **ARM64** | arm64 | Cross-compilation, Raspberry Pi 4 compatibility |
| **ARMHF** | armhf | Cross-compilation, Raspberry Pi 3 compatibility |

### DevContainer Testing

**Test All Architectures:**
```bash
# x64 testing (default)
./build.sh debug && cd build-debug && ctest

# ARM64 testing
# Switch to arm64 container, then:
./build.sh debug && cd build-debug && ctest

# ARMHF testing  
# Switch to armhf container, then:
./build.sh debug && cd build-debug && ctest
```

### Emulation Testing

**QEMU Testing for ARM:**
```bash
# Install QEMU
sudo apt install qemu-user-static

# Test ARM64 binary on x64
qemu-aarch64-static ./build-arm64/test_aasdk_transport

# Test ARMHF binary on x64
qemu-arm-static ./build-armhf/test_aasdk_transport
```

---

## Automated Testing

### GitHub Actions CI

**Create `.github/workflows/ci.yml`:**
```yaml
name: CI

on: [push, pull_request]

jobs:
  test:
    runs-on: ubuntu-latest
    strategy:
      matrix:
        architecture: [amd64, arm64, armhf]
        build_type: [Debug, Release]
    
    steps:
    - uses: actions/checkout@v3
    
    - name: Set up QEMU
      uses: docker/setup-qemu-action@v2
      
    - name: Set up Docker Buildx
      uses: docker/setup-buildx-action@v2
      
    - name: Build and Test
      run: |
        docker run --rm -v $PWD:/workspace \
          ghcr.io/opencardev/aasdk-build:${{ matrix.architecture }} \
          bash -c "cd /workspace && ./build.sh debug && cd build-debug && ctest"
```

### Pre-commit Hooks

**Install pre-commit:**
```bash
pip install pre-commit

# Create .pre-commit-config.yaml
cat > .pre-commit-config.yaml << EOF
repos:
  - repo: local
    hooks:
      - id: cpp-tests
        name: C++ Unit Tests
        entry: bash -c 'cd build-debug && ctest --output-on-failure'
        language: system
        pass_filenames: false
        
      - id: memory-check
        name: Memory Leak Check
        entry: bash -c 'valgrind --error-exitcode=1 --leak-check=full ./build-debug/test_aasdk_common'
        language: system
        pass_filenames: false
EOF

# Install hooks
pre-commit install
```

---

## Manual Testing Procedures

### Device Connection Testing

**USB Device Testing:**
1. Connect Android device via USB
2. Enable Developer Options and USB Debugging
3. Launch AASDK application
4. Verify device detection in logs:
   ```
   [INFO][USB] Device detected: Vendor=18d1 Product=4ee7
   [INFO][TRANSPORT] USB transport started successfully
   ```

**TCP Connection Testing:**
1. Enable Android Auto wireless mode
2. Connect phone to same WiFi network
3. Start TCP transport on port 5277
4. Verify connection establishment:
   ```
   [INFO][TCP] Listening on port 5277
   [INFO][TCP] Client connected from 192.168.1.100
   ```

### Audio/Video Testing

**Audio Playback Test:**
1. Start music playback on Android device
2. Monitor audio channel logs:
   ```
   [DEBUG][AUDIO_MEDIA] Audio frame received: 1024 bytes
   [DEBUG][AUDIO_MEDIA] Sample rate: 48000 Hz, Channels: 2
   ```
3. Verify audio output on speakers/headphones

**Video Display Test:**
1. Launch Android Auto on device
2. Monitor video channel:
   ```
   [DEBUG][VIDEO_SINK] Video frame received: 1920x1080 @ 60fps
   [DEBUG][VIDEO_SINK] H.264 codec, bitrate: 2000 kbps
   ```
3. Verify video display on screen

### Error Recovery Testing

**USB Disconnection Test:**
1. Connect device via USB
2. Establish AndroidAuto session
3. Physically disconnect USB cable
4. Verify graceful error handling:
   ```
   [WARN][USB] Device disconnected unexpectedly
   [INFO][TRANSPORT] Attempting reconnection...
   ```

**Network Interruption Test:**
1. Establish wireless AndroidAuto session
2. Disable WiFi on phone temporarily
3. Re-enable WiFi
4. Verify session recovery:
   ```
   [WARN][TCP] Connection lost, retrying...
   [INFO][TCP] Connection restored successfully
   ```

---

## Test Data and Fixtures

### Mock Data Generation

**Audio Test Data:**
```cpp
class AudioTestData {
public:
    static aasdk::audio::AudioData generateSineWave(int frequency, int sampleRate, int duration) {
        size_t samples = sampleRate * duration;
        aasdk::audio::AudioData data(samples * sizeof(int16_t));
        
        int16_t* buffer = reinterpret_cast<int16_t*>(data.data());
        
        for (size_t i = 0; i < samples; ++i) {
            double angle = 2.0 * M_PI * frequency * i / sampleRate;
            buffer[i] = static_cast<int16_t>(32767.0 * sin(angle));
        }
        
        return data;
    }
};
```

**Video Test Data:**
```cpp
class VideoTestData {
public:
    static aasdk::video::VideoFrame generateTestPattern(int width, int height) {
        aasdk::video::VideoFrame frame(width * height * 3); // RGB24
        
        uint8_t* buffer = frame.data();
        
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                int offset = (y * width + x) * 3;
                
                // Generate color bars pattern
                buffer[offset] = (x * 255) / width;     // Red
                buffer[offset + 1] = (y * 255) / height; // Green  
                buffer[offset + 2] = 128;                // Blue
            }
        }
        
        return frame;
    }
};
```

### Test Configuration Files

**Test Configuration JSON:**
```json
{
  "test_config": {
    "usb": {
      "vendor_id": "0x18d1",
      "product_id": "0x4ee7",
      "timeout_ms": 5000
    },
    "tcp": {
      "port": 5277,
      "bind_address": "0.0.0.0",
      "connection_timeout_ms": 10000
    },
    "audio": {
      "sample_rate": 48000,
      "channels": 2,
      "buffer_size": 1024
    },
    "video": {
      "resolution": "1920x1080",
      "framerate": 60,
      "codec": "h264"
    }
  }
}
```

---

## Test Results and Reporting

### Test Report Format

**JUnit XML Output:**
```bash
# Generate JUnit XML reports
ctest --output-junit test_results.xml

# View in CI systems or test reporting tools
```

**Custom Test Report:**
```cpp
class TestReporter {
public:
    struct TestResult {
        std::string testName;
        bool passed;
        std::chrono::milliseconds duration;
        std::string errorMessage;
    };
    
    void generateReport(const std::vector<TestResult>& results) {
        // Generate HTML/JSON test report
    }
};
```

### Performance Benchmarks

**Baseline Performance Metrics:**
- **USB Throughput:** > 480 Mbps (USB 2.0)
- **TCP Throughput:** > 100 Mbps (1Gbps network)
- **Audio Latency:** < 50ms end-to-end
- **Video Latency:** < 100ms end-to-end
- **Memory Usage:** < 128MB steady state
- **CPU Usage:** < 25% single core

---

*This testing guide ensures comprehensive validation of AASDK functionality across all supported platforms and use cases.*
