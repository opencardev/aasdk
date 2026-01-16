/*
*  This file is part of aasdk library project.
*  Copyright (C) 2025 OpenCarDev Team
*
*  aasdk is free software: you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 3 of the License, or
*  (at your option) any later version.

*  aasdk is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with aasdk. If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <memory>
#include <string>
#include <ostream>
#include <fstream>
#include <mutex>
#include <thread>
#include <queue>
#include <condition_variable>
#include <atomic>
#include <chrono>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <map>
#include <functional>

namespace aasdk::common {

/**
 * @brief Modern logging levels with detailed categorization
 */
enum class LogLevel {
    TRACE = 0,
    DEBUG = 1,
    INFO = 2,
    WARN = 3,
    ERROR = 4,
    FATAL = 5
};

/**
 * @brief Log categories specific to AASDK components
 */
enum class LogCategory {
    SYSTEM,
    TRANSPORT,
    CHANNEL,
    USB,
    TCP,
    MESSENGER,
    PROTOCOL,
    AUDIO,
    VIDEO,
    INPUT,
    SENSOR,
    BLUETOOTH,
    WIFI,
    GENERAL,
    
    // Detailed Channel Categories
    CHANNEL_CONTROL,
    CHANNEL_BLUETOOTH,
    CHANNEL_MEDIA_SINK,
    CHANNEL_MEDIA_SOURCE,
    CHANNEL_INPUT_SOURCE,
    CHANNEL_SENSOR_SOURCE,
    CHANNEL_NAVIGATION,
    CHANNEL_PHONE_STATUS,
    CHANNEL_RADIO,
    CHANNEL_NOTIFICATION,
    CHANNEL_VENDOR_EXT,
    CHANNEL_WIFI_PROJECTION,
    CHANNEL_MEDIA_BROWSER,
    CHANNEL_PLAYBACK_STATUS,
    
    // Audio Channel Categories  
    AUDIO_GUIDANCE,
    AUDIO_MEDIA,
    AUDIO_SYSTEM,
    AUDIO_TELEPHONY,
    AUDIO_MICROPHONE,
    
    // Video Channel Categories
    VIDEO_SINK,
    VIDEO_CHANNEL
};

/**
 * @brief Log entry structure containing all relevant information
 */
struct LogEntry {
    std::chrono::system_clock::time_point timestamp;
    LogLevel level;
    LogCategory category;
    std::string component;
    std::string function;
    std::string file;
    int line;
    std::thread::id threadId;
    std::string message;
    std::map<std::string, std::string> context;
};

/**
 * @brief Log formatter interface for customizable output formats
 */
class LogFormatter {
public:
    virtual ~LogFormatter() = default;
    virtual std::string format(const LogEntry& entry) = 0;
};

/**
 * @brief Log sink interface for customizable output destinations
 */
class LogSink {
public:
    virtual ~LogSink() = default;
    virtual void write(const std::string& formatted_message) = 0;
    virtual void flush() = 0;
};

/**
 * @brief Modern logger for AASDK with comprehensive features
 */
class ModernLogger {
public:
    using Pointer = std::shared_ptr<ModernLogger>;
    
    static ModernLogger& getInstance();
    
    ~ModernLogger();
    
    // Configuration
    void setLevel(LogLevel level);
    void setCategoryLevel(LogCategory category, LogLevel level);
    void addSink(std::shared_ptr<LogSink> sink);
    void setFormatter(std::shared_ptr<LogFormatter> formatter);
    void setAsync(bool async);
    void setMaxQueueSize(size_t maxSize);
    
    // Logging methods
    void log(LogLevel level, LogCategory category, const std::string& component,
             const std::string& function, const std::string& file, int line,
             const std::string& message);
    
    void logWithContext(LogLevel level, LogCategory category, const std::string& component,
                       const std::string& function, const std::string& file, int line,
                       const std::string& message, const std::map<std::string, std::string>& context);
    
    // Convenience methods
    void trace(LogCategory category, const std::string& component, const std::string& function,
               const std::string& file, int line, const std::string& message);
    void debug(LogCategory category, const std::string& component, const std::string& function,
               const std::string& file, int line, const std::string& message);
    void info(LogCategory category, const std::string& component, const std::string& function,
              const std::string& file, int line, const std::string& message);
    void warn(LogCategory category, const std::string& component, const std::string& function,
              const std::string& file, int line, const std::string& message);
    void error(LogCategory category, const std::string& component, const std::string& function,
               const std::string& file, int line, const std::string& message);
    void fatal(LogCategory category, const std::string& component, const std::string& function,
               const std::string& file, int line, const std::string& message);
    
    // Control
    void flush();
    void shutdown();
    
    // Status
    size_t getQueueSize() const;
    size_t getDroppedMessages() const;
    
    // Public method for checking if logging should happen
    bool shouldLog(LogLevel level, LogCategory category) const;
    // Utility methods
    static std::string levelToString(LogLevel level);
    static std::string categoryToString(LogCategory category);
    static LogLevel stringToLevel(const std::string& level);
    static LogCategory stringToCategory(const std::string& category);

private:
    ModernLogger();
    ModernLogger(const ModernLogger&) = delete;
    ModernLogger& operator=(const ModernLogger&) = delete;
    
    void processLogs();
    
    mutable std::mutex mutex_;
    LogLevel globalLevel_;
    std::map<LogCategory, LogLevel> categoryLevels_;
    std::vector<std::shared_ptr<LogSink>> sinks_;
    std::shared_ptr<LogFormatter> formatter_;
    
    // Async processing
    bool async_;
    size_t maxQueueSize_;
    std::queue<LogEntry> logQueue_;
    std::thread workerThread_;
    std::condition_variable condition_;
    std::atomic<bool> shutdown_;
    std::atomic<size_t> droppedMessages_;
};

/**
 * @brief Console formatter for AASDK logs
 */
class AasdkConsoleFormatter : public LogFormatter {
public:
    AasdkConsoleFormatter(bool useColors = true, bool showThreadId = false, bool showLocation = true);
    
    std::string format(const LogEntry& entry) override;

private:
    std::string getLevelColor(LogLevel level) const;
    std::string getCategoryColor(LogCategory category) const;
    std::string resetColor() const;
    
    bool useColors_;
    bool showThreadId_;
    bool showLocation_;
};

/**
 * @brief JSON formatter for structured logging
 */
class JsonFormatter : public LogFormatter {
public:
    JsonFormatter(bool prettyPrint = false);
    
    std::string format(const LogEntry& entry) override;

private:
    bool prettyPrint_;
};

/**
 * @brief File formatter optimized for log files
 */
class FileFormatter : public LogFormatter {
public:
    FileFormatter();
    
    std::string format(const LogEntry& entry) override;
};

/**
 * @brief Console sink for stdout/stderr output
 */
class ConsoleSink : public LogSink {
public:
    ConsoleSink(bool useStderr = false);
    
    void write(const std::string& message) override;
    void flush() override;

private:
    bool useStderr_;
};

/**
 * @brief File sink with rotation support
 */
class FileSink : public LogSink {
public:
    FileSink(const std::string& filename, size_t maxSize = 10 * 1024 * 1024, size_t maxFiles = 5);
    ~FileSink();
    
    void write(const std::string& message) override;
    void flush() override;

private:
    void rotateFile();
    
    std::string filename_;
    std::ofstream file_;
    std::mutex fileMutex_;
    size_t maxSize_;
    size_t maxFiles_;
    size_t currentSize_;
};

/**
 * @brief Remote sink for network logging
 */
class RemoteSink : public LogSink {
public:
    RemoteSink(const std::string& endpoint);
    
    void write(const std::string& message) override;
    void flush() override;

private:
    std::string endpoint_;
    std::queue<std::string> pendingMessages_;
    std::mutex queueMutex_;
};

} // namespace common

// Convenience macros for AASDK logging
#define AASDK_LOG_TRACE(category, message) \
    aasdk::common::ModernLogger::getInstance().trace( \
        aasdk::common::LogCategory::category, \
        __PRETTY_FUNCTION__, \
        __FUNCTION__, \
        __FILE__, \
        __LINE__, \
        message)

#define AASDK_LOG_DEBUG(category, message) \
    aasdk::common::ModernLogger::getInstance().debug( \
        aasdk::common::LogCategory::category, \
        __PRETTY_FUNCTION__, \
        __FUNCTION__, \
        __FILE__, \
        __LINE__, \
        message)

#define AASDK_LOG_INFO(category, message) \
    aasdk::common::ModernLogger::getInstance().info( \
        aasdk::common::LogCategory::category, \
        __PRETTY_FUNCTION__, \
        __FUNCTION__, \
        __FILE__, \
        __LINE__, \
        message)

#define AASDK_LOG_WARN(category, message) \
    aasdk::common::ModernLogger::getInstance().warn( \
        aasdk::common::LogCategory::category, \
        __PRETTY_FUNCTION__, \
        __FUNCTION__, \
        __FILE__, \
        __LINE__, \
        message)

#define AASDK_LOG_ERROR(category, message) \
    aasdk::common::ModernLogger::getInstance().error( \
        aasdk::common::LogCategory::category, \
        __PRETTY_FUNCTION__, \
        __FUNCTION__, \
        __FILE__, \
        __LINE__, \
        message)

#define AASDK_LOG_FATAL(category, message) \
    aasdk::common::ModernLogger::getInstance().fatal( \
        aasdk::common::LogCategory::category, \
        __PRETTY_FUNCTION__, \
        __FUNCTION__, \
        __FILE__, \
        __LINE__, \
        message)
