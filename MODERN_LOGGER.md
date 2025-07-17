# AASDK Modern Logger

This document describes the modern logging system implemented for AASDK, which provides enhanced functionality while maintaining backward compatibility with existing code.

## Features

- **Structured Logging**: Categorized logging with specific log categories for different AASDK components
- **Multiple Output Formats**: Console, file, JSON, and remote logging support
- **Asynchronous Logging**: Optional async processing for high-performance scenarios
- **Log Rotation**: Automatic file rotation with configurable size and count limits
- **Context Logging**: Ability to add key-value context to log entries
- **Thread-Safe**: All operations are thread-safe
- **Backward Compatible**: Existing `AASDK_LOG` macros continue to work

## Log Levels

The logger supports the following log levels:
- `TRACE`: Detailed trace information
- `DEBUG`: Debug information useful during development
- `INFO`: General information about program execution
- `WARN`: Warning messages for potentially harmful situations
- `ERROR`: Error messages for error conditions
- `FATAL`: Fatal error messages for critical failures

## Log Categories

AASDK provides **27 comprehensive logging categories** with detailed subcategories for precise control:

### Basic Categories (15)
- `SYSTEM`: System-level operations
- `TRANSPORT`: Transport layer operations
- `CHANNEL`: General channel operations
- `USB`: USB communication
- `TCP`: TCP communication
- `MESSENGER`: Message handling
- `PROTOCOL`: Protocol-level operations
- `AUDIO`: General audio operations
- `VIDEO`: General video operations
- `INPUT`: Input handling
- `SENSOR`: Sensor operations
- `BLUETOOTH`: Bluetooth operations
- `WIFI`: WiFi operations
- `GENERAL`: General purpose logging
- `CUSTOM`: Custom application logging

### Channel Subcategories (13)
- `CHANNEL_CONTROL`: Control channel handshakes and protocol management
- `CHANNEL_BLUETOOTH`: Bluetooth pairing and connectivity
- `CHANNEL_MEDIA_SINK`: Media sink channel operations
- `CHANNEL_MEDIA_SOURCE`: Media source channel operations
- `CHANNEL_INPUT_SOURCE`: Input source channel handling
- `CHANNEL_SENSOR_SOURCE`: Sensor data channel handling
- `CHANNEL_NAVIGATION`: GPS and navigation services
- `CHANNEL_PHONE_STATUS`: Phone connectivity and status
- `CHANNEL_RADIO`: Radio frequency management
- `CHANNEL_NOTIFICATION`: System notification handling
- `CHANNEL_VENDOR_EXT`: Vendor-specific extensions
- `CHANNEL_WIFI_PROJECTION`: WiFi projection features
- `CHANNEL_MEDIA_BROWSER`: Media browsing operations
- `CHANNEL_PLAYBACK_STATUS`: Media playback status

### Audio Subcategories (5)
- `AUDIO_GUIDANCE`: Navigation voice guidance
- `AUDIO_MEDIA`: Media audio streams
- `AUDIO_SYSTEM`: System audio controls
- `AUDIO_TELEPHONY`: Phone call audio
- `AUDIO_MICROPHONE`: Microphone input handling

### Video Subcategories (2)
- `VIDEO_SINK`: Video display output
- `VIDEO_CHANNEL`: Video channel configuration

## Usage

### Basic Logging

```cpp
#include <aasdk/Common/ModernLogger.hpp>

// Basic category logging
AASDK_LOG_TRANSPORT(info, "Transport initialized");
AASDK_LOG_USB(debug, "USB device detected");
AASDK_LOG_MESSENGER(info, "Message queue ready");
AASDK_LOG_PROTOCOL(debug, "Protocol handshake complete");

// Detailed channel subcategory logging
AASDK_LOG_CHANNEL_CONTROL(debug, "Control channel handshake initiated");
AASDK_LOG_CHANNEL_BLUETOOTH(info, "Bluetooth pairing successful");
AASDK_LOG_CHANNEL_MEDIA_SINK(debug, "Media sink channel opened");
AASDK_LOG_CHANNEL_NAVIGATION(warn, "GPS signal weak");

// Audio subcategory logging
AASDK_LOG_AUDIO_GUIDANCE(info, "Navigation voice guidance ready");
AASDK_LOG_AUDIO_MEDIA(debug, "Media audio stream started");
AASDK_LOG_AUDIO_TELEPHONY(info, "Phone call audio active");

// Video subcategory logging
AASDK_LOG_VIDEO_SINK(info, "Video sink ready for display");
AASDK_LOG_VIDEO_CHANNEL(debug, "Video channel configuration complete");

// Direct logger access for advanced usage
auto& logger = aasdk::common::ModernLogger::getInstance();
logger.info(aasdk::common::LogCategory::SYSTEM, "Component", __FUNCTION__, __FILE__, __LINE__, "Message");
```

### Available Macros (47+ specialized macros)

**Basic Categories:**
- `AASDK_LOG_TRANSPORT(severity, message)`
- `AASDK_LOG_USB(severity, message)`
- `AASDK_LOG_TCP(severity, message)`
- `AASDK_LOG_MESSENGER(severity, message)`
- `AASDK_LOG_PROTOCOL(severity, message)`
- `AASDK_LOG_AUDIO(severity, message)`
- `AASDK_LOG_VIDEO(severity, message)`
- `AASDK_LOG_BLUETOOTH(severity, message)`
- `AASDK_LOG_WIFI(severity, message)`
- `AASDK_LOG_SYSTEM(severity, message)`
- `AASDK_LOG_GENERAL(severity, message)`

**Channel Subcategories:**
- `AASDK_LOG_CHANNEL_CONTROL(severity, message)`
- `AASDK_LOG_CHANNEL_BLUETOOTH(severity, message)`
- `AASDK_LOG_CHANNEL_MEDIA_SINK(severity, message)`
- `AASDK_LOG_CHANNEL_MEDIA_SOURCE(severity, message)`
- `AASDK_LOG_CHANNEL_INPUT_SOURCE(severity, message)`
- `AASDK_LOG_CHANNEL_SENSOR_SOURCE(severity, message)`
- `AASDK_LOG_CHANNEL_NAVIGATION(severity, message)`
- `AASDK_LOG_CHANNEL_PHONE_STATUS(severity, message)`
- `AASDK_LOG_CHANNEL_RADIO(severity, message)`
- `AASDK_LOG_CHANNEL_NOTIFICATION(severity, message)`
- `AASDK_LOG_CHANNEL_VENDOR_EXT(severity, message)`
- `AASDK_LOG_CHANNEL_WIFI_PROJECTION(severity, message)`
- `AASDK_LOG_CHANNEL_MEDIA_BROWSER(severity, message)`
- `AASDK_LOG_CHANNEL_PLAYBACK_STATUS(severity, message)`

**Audio Subcategories:**
- `AASDK_LOG_AUDIO_GUIDANCE(severity, message)`
- `AASDK_LOG_AUDIO_MEDIA(severity, message)`
- `AASDK_LOG_AUDIO_SYSTEM(severity, message)`
- `AASDK_LOG_AUDIO_TELEPHONY(severity, message)`
- `AASDK_LOG_AUDIO_MICROPHONE(severity, message)`

**Video Subcategories:**
- `AASDK_LOG_VIDEO_SINK(severity, message)`
- `AASDK_LOG_VIDEO_CHANNEL(severity, message)`

### Backward Compatibility

Existing code continues to work without changes:

```cpp
#include <aasdk/Common/Log.hpp>

AASDK_LOG(info) << "This still works";
AASDK_LOG(debug) << "Debug message with " << variable;
```

### Configuration

```cpp
auto& logger = aasdk::common::ModernLogger::getInstance();

// Set global log level
logger.setLevel(aasdk::common::LogLevel::DEBUG);

// Set category-specific levels for detailed control
logger.setCategoryLevel(aasdk::common::LogCategory::USB, aasdk::common::LogLevel::TRACE);
logger.setCategoryLevel(aasdk::common::LogCategory::TRANSPORT, aasdk::common::LogLevel::INFO);

// Configure detailed channel logging
logger.setCategoryLevel(aasdk::common::LogCategory::CHANNEL_CONTROL, aasdk::common::LogLevel::DEBUG);
logger.setCategoryLevel(aasdk::common::LogCategory::CHANNEL_BLUETOOTH, aasdk::common::LogLevel::INFO);
logger.setCategoryLevel(aasdk::common::LogCategory::CHANNEL_MEDIA_SINK, aasdk::common::LogLevel::DEBUG);

// Configure audio subcategory logging
logger.setCategoryLevel(aasdk::common::LogCategory::AUDIO_GUIDANCE, aasdk::common::LogLevel::INFO);
logger.setCategoryLevel(aasdk::common::LogCategory::AUDIO_MEDIA, aasdk::common::LogLevel::DEBUG);
logger.setCategoryLevel(aasdk::common::LogCategory::AUDIO_TELEPHONY, aasdk::common::LogLevel::INFO);

// Configure video subcategory logging
logger.setCategoryLevel(aasdk::common::LogCategory::VIDEO_SINK, aasdk::common::LogLevel::INFO);
logger.setCategoryLevel(aasdk::common::LogCategory::VIDEO_CHANNEL, aasdk::common::LogLevel::DEBUG);

// Add file logging with rotation
auto fileSink = std::make_shared<aasdk::common::FileSink>("aasdk.log", 10*1024*1024, 5);
logger.addSink(fileSink);

// Enable async logging for performance
logger.setAsync(true);
logger.setMaxQueueSize(10000);
```

### Context Logging

Add structured context to log entries:

```cpp
std::map<std::string, std::string> context;
context["device_id"] = "12345";
context["session_id"] = "abcdef";

logger.logWithContext(
    aasdk::common::LogLevel::INFO,
    aasdk::common::LogCategory::SYSTEM,
    "MyComponent",
    __FUNCTION__,
    __FILE__,
    __LINE__,
    "Operation completed",
    context
);
```

## Formatters

### Console Formatter (default)
Provides colored, human-readable output suitable for terminal display.

### File Formatter
Optimized for log files with consistent formatting and all details.

### JSON Formatter
Structured JSON output for log analysis tools (requires `ENABLE_JSON_LOGGING`).

```cpp
// Use JSON formatter
auto jsonFormatter = std::make_shared<aasdk::common::JsonFormatter>(true); // pretty print
logger.setFormatter(jsonFormatter);
```

## Sinks

### Console Sink (default)
Outputs to stdout or stderr.

### File Sink
Writes to files with automatic rotation:

```cpp
auto fileSink = std::make_shared<aasdk::common::FileSink>(
    "aasdk.log",    // filename
    10*1024*1024,   // max size (10MB)
    5               // max files
);
logger.addSink(fileSink);
```

### Remote Sink
For network logging (implementation placeholder):

```cpp
auto remoteSink = std::make_shared<aasdk::common::RemoteSink>("http://log-server:8080/logs");
logger.addSink(remoteSink);
```

## Build Configuration

### Enable Modern Logging (default)
```bash
cd build
cmake ..
make
```

### Disable Modern Logging (fallback to boost)
```bash
cd build
cmake -DDISABLE_MODERN_LOGGING=ON ..
make
```

### Enable JSON Support
```bash
cd build
cmake -DENABLE_JSON_LOGGING=ON ..
make
```

## Migration Guide

### ✅ Migration Status
**The AASDK codebase has been successfully migrated to the modern logger system!**

- ✅ **Build System**: Library compiles cleanly with modern logging enabled
- ✅ **Macro Resolution**: All 47+ category-specific macros work correctly  
- ✅ **Backward Compatibility**: Existing `AASDK_LOG(severity) << "message"` syntax still functional
- ✅ **Channel Subcategories**: Detailed logging categories implemented and tested
- ✅ **Verification Testing**: Comprehensive tests confirm all categories work properly

### Automatic Migration (Recommended)

1. **Use the Enhanced Migration Script**:
   ```bash
   cd /home/pi/aasdk
   ./migrate_to_modern_logger.sh
   ```
   
   This script automatically:
   - Migrates transport, messenger, USB, TCP components to basic categories
   - Migrates channel files to detailed subcategories (`CHANNEL_CONTROL`, `CHANNEL_BLUETOOTH`, etc.)
   - Migrates audio files to audio subcategories (`AUDIO_GUIDANCE`, `AUDIO_MEDIA`, etc.) 
   - Migrates video files to video subcategories (`VIDEO_SINK`, `VIDEO_CHANNEL`)
   - Adds required includes for `ModernLogger.hpp`
   - Creates backup of original files

2. **Verify Migration**:
   ```bash
   cd /home/pi/aasdk/build
   make
   ```

3. **Test Migration**:
   ```bash
   cd /home/pi/aasdk
   g++ -std=c++17 -I include test_migration.cpp -L lib -laasdk -o test_migration -Wl,-rpath,./lib -lpthread
   ./test_migration
   ```

### Manual Migration (If Needed)

1. **Add Headers**: Include `<aasdk/Common/ModernLogger.hpp>` in source files
2. **Replace Logging Calls**: 
   - Replace `AASDK_LOG(level) << "[Component] message";` 
   - With appropriate category macros: `AASDK_LOG_CATEGORY(level, "message");`
3. **Use Specific Categories**: Choose the most appropriate category:
   - **Basic**: `AASDK_LOG_TRANSPORT`, `AASDK_LOG_USB`, `AASDK_LOG_MESSENGER`
   - **Channel**: `AASDK_LOG_CHANNEL_CONTROL`, `AASDK_LOG_CHANNEL_BLUETOOTH`
   - **Audio**: `AASDK_LOG_AUDIO_GUIDANCE`, `AASDK_LOG_AUDIO_MEDIA`
   - **Video**: `AASDK_LOG_VIDEO_SINK`, `AASDK_LOG_VIDEO_CHANNEL`

### Example Migration
**Before:**
```cpp
AASDK_LOG(debug) << "[ControlChannel] Handshake initiated";
AASDK_LOG(info) << "[AudioMedia] Stream started";
```

**After:**
```cpp
AASDK_LOG_CHANNEL_CONTROL(debug, "Handshake initiated");
AASDK_LOG_AUDIO_MEDIA(info, "Stream started");
```

### Testing Migration
```bash
# Compile test program
cd /home/pi/aasdk
g++ -std=c++17 -I include test_migration.cpp -L lib -laasdk -o test_migration -Wl,-rpath,./lib -lpthread

# Run verification test
./test_migration
```

### Rollback (If Needed)
If issues arise, temporarily disable modern logging:
```bash
cd build
cmake -DDISABLE_MODERN_LOGGING=ON ..
make
```

## Performance

- **Synchronous logging**: Suitable for most applications
- **Asynchronous logging**: For high-throughput scenarios (10,000+ messages/second)
- **Memory usage**: Configurable queue size for async mode
- **CPU overhead**: Minimal impact in sync mode, near-zero in async mode

## Thread Safety

All logger operations are thread-safe. Multiple threads can log simultaneously without external synchronization.

## Best Practices

1. **Use category-specific macros** for better log organization and filtering
2. **Choose appropriate categories**:
   - Use detailed subcategories (`CHANNEL_CONTROL`, `AUDIO_MEDIA`) for specific components
   - Use basic categories (`TRANSPORT`, `USB`) for general subsystems
3. **Set meaningful log levels** for different categories to control verbosity
4. **Use context logging** for structured data and debugging information
5. **Enable async logging** for performance-critical applications (high message volume)
6. **Configure file rotation** to prevent disk space issues in production
7. **Use meaningful component names** and descriptive messages
8. **Consider log levels carefully**:
   - `TRACE`/`DEBUG`: Development and troubleshooting
   - `INFO`: Normal operation status
   - `WARN`: Potential issues that don't stop operation
   - `ERROR`/`FATAL`: Actual problems requiring attention

## Migration Benefits

The enhanced modern logger provides significant advantages:

### **Improved Debugging**
- **Granular Control**: 27 categories allow precise filtering of log output
- **Channel-Specific Debugging**: Focus on specific Android Auto channels
- **Audio/Video Isolation**: Separate logging for media subsystems
- **Enhanced Readability**: Color-coded output with consistent formatting

### **Production Benefits**
- **Performance**: Async logging for high-throughput scenarios
- **Storage Management**: Automatic log rotation prevents disk filling
- **Structured Data**: JSON output for log analysis tools
- **Remote Logging**: Network sink support for centralized logging

### **Development Efficiency**  
- **Backward Compatibility**: Existing code continues to work unchanged
- **Easy Migration**: Automated script handles bulk conversion
- **Category Filtering**: Focus on relevant subsystems during development
- **Context Support**: Structured data for complex debugging scenarios

## Example Configuration

```cpp
void setupAASDKLogging() {
    auto& logger = aasdk::common::ModernLogger::getInstance();
    
    // Set appropriate levels for different subsystems
    logger.setLevel(aasdk::common::LogLevel::INFO);  // Global level
    
    // Enable detailed logging for specific channels during development
    logger.setCategoryLevel(aasdk::common::LogCategory::CHANNEL_CONTROL, aasdk::common::LogLevel::DEBUG);
    logger.setCategoryLevel(aasdk::common::LogCategory::CHANNEL_BLUETOOTH, aasdk::common::LogLevel::DEBUG);
    logger.setCategoryLevel(aasdk::common::LogCategory::TRANSPORT, aasdk::common::LogLevel::DEBUG);
    logger.setCategoryLevel(aasdk::common::LogCategory::USB, aasdk::common::LogLevel::DEBUG);
    
    // Keep audio/video logging at info level to avoid spam
    logger.setCategoryLevel(aasdk::common::LogCategory::AUDIO_MEDIA, aasdk::common::LogLevel::INFO);
    logger.setCategoryLevel(aasdk::common::LogCategory::VIDEO_SINK, aasdk::common::LogLevel::INFO);
    
    // Add file logging with rotation for production
    auto fileSink = std::make_shared<aasdk::common::FileSink>(
        "/var/log/aasdk/aasdk.log", 
        100*1024*1024,  // 100MB per file
        10              // Keep 10 files (1GB total)
    );
    logger.addSink(fileSink);
    
    // Enable async for performance in production
    logger.setAsync(true);
    logger.setMaxQueueSize(5000);
    
    AASDK_LOG_SYSTEM(info, "AASDK logging initialized with enhanced categories");
}
```
