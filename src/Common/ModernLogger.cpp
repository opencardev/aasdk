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

#include "aasdk/Common/ModernLogger.hpp"
#include <filesystem>
#include <regex>

#ifdef ENABLE_JSON_LOGGING
#include <nlohmann/json.hpp>
#endif

namespace aasdk {
namespace common {

// ModernLogger Implementation
ModernLogger& ModernLogger::getInstance() {
    static ModernLogger instance;
    return instance;
}

ModernLogger::ModernLogger() 
    : globalLevel_(LogLevel::INFO)
    , async_(false)
    , maxQueueSize_(10000)
    , shutdown_(false)
    , droppedMessages_(0) {
    
    // Set up default console formatter and sink
    formatter_ = std::make_shared<AasdkConsoleFormatter>();
    addSink(std::make_shared<ConsoleSink>());
}

ModernLogger::~ModernLogger() {
    shutdown();
}

void ModernLogger::setLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(mutex_);
    globalLevel_ = level;
}

void ModernLogger::setCategoryLevel(LogCategory category, LogLevel level) {
    std::lock_guard<std::mutex> lock(mutex_);
    categoryLevels_[category] = level;
}

void ModernLogger::addSink(std::shared_ptr<LogSink> sink) {
    std::lock_guard<std::mutex> lock(mutex_);
    sinks_.push_back(sink);
}

void ModernLogger::setFormatter(std::shared_ptr<LogFormatter> formatter) {
    std::lock_guard<std::mutex> lock(mutex_);
    formatter_ = formatter;
}

void ModernLogger::setAsync(bool async) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (async && !async_) {
        // Start async processing
        async_ = true;
        shutdown_ = false;
        workerThread_ = std::thread(&ModernLogger::processLogs, this);
    } else if (!async && async_) {
        // Stop async processing
        async_ = false;
        shutdown_ = true;
        condition_.notify_all();
        
        if (workerThread_.joinable()) {
            workerThread_.join();
        }
    }
}

void ModernLogger::setMaxQueueSize(size_t maxSize) {
    std::lock_guard<std::mutex> lock(mutex_);
    maxQueueSize_ = maxSize;
}

void ModernLogger::log(LogLevel level, LogCategory category, const std::string& component,
                const std::string& function, const std::string& file, int line,
                const std::string& message) {
    
    if (!shouldLog(level, category)) {
        return;
    }
    
    LogEntry entry;
    entry.timestamp = std::chrono::system_clock::now();
    entry.level = level;
    entry.category = category;
    entry.component = component;
    entry.function = function;
    entry.file = file;
    entry.line = line;
    entry.threadId = std::this_thread::get_id();
    entry.message = message;
    
    if (async_) {
        std::unique_lock<std::mutex> lock(mutex_);
        
        if (logQueue_.size() >= maxQueueSize_) {
            // Drop oldest message
            logQueue_.pop();
            droppedMessages_++;
        }
        
        logQueue_.push(entry);
        condition_.notify_one();
    } else {
        // Synchronous logging
        std::lock_guard<std::mutex> lock(mutex_);
        std::string formatted = formatter_->format(entry);
        
        for (auto& sink : sinks_) {
            sink->write(formatted);
        }
    }
}

void ModernLogger::logWithContext(LogLevel level, LogCategory category, const std::string& component,
                           const std::string& function, const std::string& file, int line,
                           const std::string& message, const std::map<std::string, std::string>& context) {
    
    if (!shouldLog(level, category)) {
        return;
    }
    
    LogEntry entry;
    entry.timestamp = std::chrono::system_clock::now();
    entry.level = level;
    entry.category = category;
    entry.component = component;
    entry.function = function;
    entry.file = file;
    entry.line = line;
    entry.threadId = std::this_thread::get_id();
    entry.message = message;
    entry.context = context;
    
    if (async_) {
        std::unique_lock<std::mutex> lock(mutex_);
        
        if (logQueue_.size() >= maxQueueSize_) {
            logQueue_.pop();
            droppedMessages_++;
        }
        
        logQueue_.push(entry);
        condition_.notify_one();
    } else {
        std::lock_guard<std::mutex> lock(mutex_);
        std::string formatted = formatter_->format(entry);
        
        for (auto& sink : sinks_) {
            sink->write(formatted);
        }
    }
}

void ModernLogger::trace(LogCategory category, const std::string& component, const std::string& function,
                  const std::string& file, int line, const std::string& message) {
    log(LogLevel::TRACE, category, component, function, file, line, message);
}

void ModernLogger::debug(LogCategory category, const std::string& component, const std::string& function,
                  const std::string& file, int line, const std::string& message) {
    log(LogLevel::DEBUG, category, component, function, file, line, message);
}

void ModernLogger::info(LogCategory category, const std::string& component, const std::string& function,
                 const std::string& file, int line, const std::string& message) {
    log(LogLevel::INFO, category, component, function, file, line, message);
}

void ModernLogger::warn(LogCategory category, const std::string& component, const std::string& function,
                 const std::string& file, int line, const std::string& message) {
    log(LogLevel::WARN, category, component, function, file, line, message);
}

void ModernLogger::error(LogCategory category, const std::string& component, const std::string& function,
                  const std::string& file, int line, const std::string& message) {
    log(LogLevel::ERROR, category, component, function, file, line, message);
}

void ModernLogger::fatal(LogCategory category, const std::string& component, const std::string& function,
                  const std::string& file, int line, const std::string& message) {
    log(LogLevel::FATAL, category, component, function, file, line, message);
}

void ModernLogger::flush() {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& sink : sinks_) {
        sink->flush();
    }
}

void ModernLogger::shutdown() {
    if (async_ && !shutdown_.load()) {
        shutdown_ = true;
        condition_.notify_all();
        
        if (workerThread_.joinable()) {
            workerThread_.join();
        }
    }
    
    flush();
}

size_t ModernLogger::getQueueSize() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return logQueue_.size();
}

size_t ModernLogger::getDroppedMessages() const {
    return droppedMessages_.load();
}

void ModernLogger::processLogs() {
    while (!shutdown_.load()) {
        std::unique_lock<std::mutex> lock(mutex_);
        
        condition_.wait(lock, [this] {
            return !logQueue_.empty() || shutdown_.load();
        });
        
        while (!logQueue_.empty()) {
            LogEntry entry = logQueue_.front();
            logQueue_.pop();
            
            std::string formatted = formatter_->format(entry);
            
            for (auto& sink : sinks_) {
                sink->write(formatted);
            }
        }
    }
}

bool ModernLogger::shouldLog(LogLevel level, LogCategory category) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check category-specific level first
    auto it = categoryLevels_.find(category);
    if (it != categoryLevels_.end()) {
        return level >= it->second;
    }
    
    // Fall back to global level
    return level >= globalLevel_;
}

std::string ModernLogger::levelToString(LogLevel level) {
    switch (level) {
        case LogLevel::TRACE: return "TRACE";
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO:  return "INFO";
        case LogLevel::WARN:  return "WARN";
        case LogLevel::ERROR: return "ERROR";
        case LogLevel::FATAL: return "FATAL";
        default: return "UNKNOWN";
    }
}

std::string ModernLogger::categoryToString(LogCategory category) {
    switch (category) {
        case LogCategory::SYSTEM:    return "SYSTEM";
        case LogCategory::TRANSPORT: return "TRANSPORT";
        case LogCategory::CHANNEL:   return "CHANNEL";
        case LogCategory::USB:       return "USB";
        case LogCategory::TCP:       return "TCP";
        case LogCategory::MESSENGER: return "MESSENGER";
        case LogCategory::PROTOCOL:  return "PROTOCOL";
        case LogCategory::AUDIO:     return "AUDIO";
        case LogCategory::VIDEO:     return "VIDEO";
        case LogCategory::INPUT:     return "INPUT";
        case LogCategory::SENSOR:    return "SENSOR";
        case LogCategory::BLUETOOTH: return "BLUETOOTH";
        case LogCategory::WIFI:      return "WIFI";
        case LogCategory::GENERAL:   return "GENERAL";
        
        // Detailed Channel Categories
        case LogCategory::CHANNEL_CONTROL:        return "CHANNEL_CONTROL";
        case LogCategory::CHANNEL_BLUETOOTH:      return "CHANNEL_BLUETOOTH";
        case LogCategory::CHANNEL_MEDIA_SINK:     return "CHANNEL_MEDIA_SINK";
        case LogCategory::CHANNEL_MEDIA_SOURCE:   return "CHANNEL_MEDIA_SOURCE";
        case LogCategory::CHANNEL_INPUT_SOURCE:   return "CHANNEL_INPUT_SOURCE";
        case LogCategory::CHANNEL_SENSOR_SOURCE:  return "CHANNEL_SENSOR_SOURCE";
        case LogCategory::CHANNEL_NAVIGATION:     return "CHANNEL_NAVIGATION";
        case LogCategory::CHANNEL_PHONE_STATUS:   return "CHANNEL_PHONE_STATUS";
        case LogCategory::CHANNEL_RADIO:          return "CHANNEL_RADIO";
        case LogCategory::CHANNEL_NOTIFICATION:   return "CHANNEL_NOTIFICATION";
        case LogCategory::CHANNEL_VENDOR_EXT:     return "CHANNEL_VENDOR_EXT";
        case LogCategory::CHANNEL_WIFI_PROJECTION: return "CHANNEL_WIFI_PROJECTION";
        case LogCategory::CHANNEL_MEDIA_BROWSER:  return "CHANNEL_MEDIA_BROWSER";
        case LogCategory::CHANNEL_PLAYBACK_STATUS: return "CHANNEL_PLAYBACK_STATUS";
        
        // Audio Channel Categories
        case LogCategory::AUDIO_GUIDANCE:    return "AUDIO_GUIDANCE";
        case LogCategory::AUDIO_MEDIA:       return "AUDIO_MEDIA";
        case LogCategory::AUDIO_SYSTEM:      return "AUDIO_SYSTEM";
        case LogCategory::AUDIO_TELEPHONY:   return "AUDIO_TELEPHONY";
        case LogCategory::AUDIO_MICROPHONE:  return "AUDIO_MICROPHONE";
        
        // Video Channel Categories
        case LogCategory::VIDEO_SINK:        return "VIDEO_SINK";
        case LogCategory::VIDEO_CHANNEL:     return "VIDEO_CHANNEL";
        
        default: return "UNKNOWN";
    }
}

LogLevel ModernLogger::stringToLevel(const std::string& level) {
    if (level == "TRACE") return LogLevel::TRACE;
    if (level == "DEBUG") return LogLevel::DEBUG;
    if (level == "INFO")  return LogLevel::INFO;
    if (level == "WARN")  return LogLevel::WARN;
    if (level == "ERROR") return LogLevel::ERROR;
    if (level == "FATAL") return LogLevel::FATAL;
    return LogLevel::INFO; // Default
}

LogCategory ModernLogger::stringToCategory(const std::string& category) {
    if (category == "SYSTEM")    return LogCategory::SYSTEM;
    if (category == "TRANSPORT") return LogCategory::TRANSPORT;
    if (category == "CHANNEL")   return LogCategory::CHANNEL;
    if (category == "USB")       return LogCategory::USB;
    if (category == "TCP")       return LogCategory::TCP;
    if (category == "MESSENGER") return LogCategory::MESSENGER;
    if (category == "PROTOCOL")  return LogCategory::PROTOCOL;
    if (category == "AUDIO")     return LogCategory::AUDIO;
    if (category == "VIDEO")     return LogCategory::VIDEO;
    if (category == "INPUT")     return LogCategory::INPUT;
    if (category == "SENSOR")    return LogCategory::SENSOR;
    if (category == "BLUETOOTH") return LogCategory::BLUETOOTH;
    if (category == "WIFI")      return LogCategory::WIFI;
    
    // Detailed Channel Categories
    if (category == "CHANNEL_CONTROL")        return LogCategory::CHANNEL_CONTROL;
    if (category == "CHANNEL_BLUETOOTH")      return LogCategory::CHANNEL_BLUETOOTH;
    if (category == "CHANNEL_MEDIA_SINK")     return LogCategory::CHANNEL_MEDIA_SINK;
    if (category == "CHANNEL_MEDIA_SOURCE")   return LogCategory::CHANNEL_MEDIA_SOURCE;
    if (category == "CHANNEL_INPUT_SOURCE")   return LogCategory::CHANNEL_INPUT_SOURCE;
    if (category == "CHANNEL_SENSOR_SOURCE")  return LogCategory::CHANNEL_SENSOR_SOURCE;
    if (category == "CHANNEL_NAVIGATION")     return LogCategory::CHANNEL_NAVIGATION;
    if (category == "CHANNEL_PHONE_STATUS")   return LogCategory::CHANNEL_PHONE_STATUS;
    if (category == "CHANNEL_RADIO")          return LogCategory::CHANNEL_RADIO;
    if (category == "CHANNEL_NOTIFICATION")   return LogCategory::CHANNEL_NOTIFICATION;
    if (category == "CHANNEL_VENDOR_EXT")     return LogCategory::CHANNEL_VENDOR_EXT;
    if (category == "CHANNEL_WIFI_PROJECTION") return LogCategory::CHANNEL_WIFI_PROJECTION;
    if (category == "CHANNEL_MEDIA_BROWSER")  return LogCategory::CHANNEL_MEDIA_BROWSER;
    if (category == "CHANNEL_PLAYBACK_STATUS") return LogCategory::CHANNEL_PLAYBACK_STATUS;
    
    // Audio Channel Categories
    if (category == "AUDIO_GUIDANCE")    return LogCategory::AUDIO_GUIDANCE;
    if (category == "AUDIO_MEDIA")       return LogCategory::AUDIO_MEDIA;
    if (category == "AUDIO_SYSTEM")      return LogCategory::AUDIO_SYSTEM;
    if (category == "AUDIO_TELEPHONY")   return LogCategory::AUDIO_TELEPHONY;
    if (category == "AUDIO_MICROPHONE")  return LogCategory::AUDIO_MICROPHONE;
    
    // Video Channel Categories
    if (category == "VIDEO_SINK")        return LogCategory::VIDEO_SINK;
    if (category == "VIDEO_CHANNEL")     return LogCategory::VIDEO_CHANNEL;
    
    return LogCategory::GENERAL; // Default
}

// AasdkConsoleFormatter Implementation
AasdkConsoleFormatter::AasdkConsoleFormatter(bool useColors, bool showThreadId, bool showLocation)
    : useColors_(useColors), showThreadId_(showThreadId), showLocation_(showLocation) {
}

std::string AasdkConsoleFormatter::format(const LogEntry& entry) {
    std::ostringstream oss;
    
    // Timestamp
    auto time_t = std::chrono::system_clock::to_time_t(entry.timestamp);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        entry.timestamp.time_since_epoch()) % 1000;
    
    oss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    oss << "." << std::setfill('0') << std::setw(3) << ms.count();
    
    // Level with color
    if (useColors_) {
        oss << " " << getLevelColor(entry.level) << "[" << ModernLogger::levelToString(entry.level) << "]" << resetColor();
    } else {
        oss << " [" << ModernLogger::levelToString(entry.level) << "]";
    }
    
    // Category with color
    if (useColors_) {
        oss << " " << getCategoryColor(entry.category) << "[" << ModernLogger::categoryToString(entry.category) << "]" << resetColor();
    } else {
        oss << " [" << ModernLogger::categoryToString(entry.category) << "]";
    }
    
    // Thread ID
    if (showThreadId_) {
        oss << " [" << entry.threadId << "]";
    }
    
    // Component and function - clean up C++ mangled names
    std::string component = entry.component;
    if (component.find("class ") == 0) {
        component = component.substr(6);
    }
    size_t lastColon = component.find_last_of(':');
    if (lastColon != std::string::npos) {
        component = component.substr(lastColon + 1);
    }
    
    oss << " [AASDK:" << component;
    if (!entry.function.empty()) {
        oss << "::" << entry.function;
    }
    oss << "]";
    
    // Location
    if (showLocation_ && !entry.file.empty()) {
        std::string filename = entry.file;
        size_t lastSlash = filename.find_last_of("/\\");
        if (lastSlash != std::string::npos) {
            filename = filename.substr(lastSlash + 1);
        }
        oss << " (" << filename << ":" << entry.line << ")";
    }
    
    // Message
    oss << " - " << entry.message;
    
    // Context
    if (!entry.context.empty()) {
        oss << " {";
        bool first = true;
        for (const auto& [key, value] : entry.context) {
            if (!first) oss << ", ";
            oss << key << "=" << value;
            first = false;
        }
        oss << "}";
    }
    
    oss << std::endl;
    return oss.str();
}

std::string AasdkConsoleFormatter::getLevelColor(LogLevel level) const {
    if (!useColors_) return "";
    
    switch (level) {
        case LogLevel::TRACE: return "\033[37m";      // White
        case LogLevel::DEBUG: return "\033[36m";      // Cyan
        case LogLevel::INFO:  return "\033[32m";      // Green
        case LogLevel::WARN:  return "\033[33m";      // Yellow
        case LogLevel::ERROR: return "\033[31m";      // Red
        case LogLevel::FATAL: return "\033[35m";      // Magenta
        default: return "";
    }
}

std::string AasdkConsoleFormatter::getCategoryColor(LogCategory category) const {
    if (!useColors_) return "";
    
    switch (category) {
        case LogCategory::SYSTEM:    return "\033[1;34m";  // Bold Blue
        case LogCategory::TRANSPORT: return "\033[1;35m";  // Bold Magenta
        case LogCategory::CHANNEL:   return "\033[1;32m";  // Bold Green
        case LogCategory::USB:       return "\033[1;36m";  // Bold Cyan
        case LogCategory::TCP:       return "\033[1;33m";  // Bold Yellow
        case LogCategory::MESSENGER: return "\033[1;31m";  // Bold Red
        case LogCategory::PROTOCOL:  return "\033[1;37m";  // Bold White
        case LogCategory::AUDIO:     return "\033[1;32m";  // Bold Green
        case LogCategory::VIDEO:     return "\033[1;35m";  // Bold Magenta
        case LogCategory::INPUT:     return "\033[1;36m";  // Bold Cyan
        case LogCategory::SENSOR:    return "\033[1;33m";  // Bold Yellow
        case LogCategory::BLUETOOTH: return "\033[1;34m";  // Bold Blue
        case LogCategory::WIFI:      return "\033[1;31m";  // Bold Red
        case LogCategory::GENERAL:   return "\033[1;37m";  // Bold White
        
        // Channel Categories - Green variants
        case LogCategory::CHANNEL_CONTROL:        return "\033[32m";    // Green
        case LogCategory::CHANNEL_BLUETOOTH:      return "\033[94m";    // Light Blue
        case LogCategory::CHANNEL_MEDIA_SINK:     return "\033[92m";    // Light Green
        case LogCategory::CHANNEL_MEDIA_SOURCE:   return "\033[96m";    // Light Cyan
        case LogCategory::CHANNEL_INPUT_SOURCE:   return "\033[93m";    // Light Yellow
        case LogCategory::CHANNEL_SENSOR_SOURCE:  return "\033[95m";    // Light Magenta
        case LogCategory::CHANNEL_NAVIGATION:     return "\033[91m";    // Light Red
        case LogCategory::CHANNEL_PHONE_STATUS:   return "\033[94m";    // Light Blue
        case LogCategory::CHANNEL_RADIO:          return "\033[92m";    // Light Green
        case LogCategory::CHANNEL_NOTIFICATION:   return "\033[96m";    // Light Cyan
        case LogCategory::CHANNEL_VENDOR_EXT:     return "\033[93m";    // Light Yellow
        case LogCategory::CHANNEL_WIFI_PROJECTION: return "\033[95m";   // Light Magenta
        case LogCategory::CHANNEL_MEDIA_BROWSER:  return "\033[91m";    // Light Red
        case LogCategory::CHANNEL_PLAYBACK_STATUS: return "\033[94m";   // Light Blue
        
        // Audio Categories - Green/Cyan variants
        case LogCategory::AUDIO_GUIDANCE:    return "\033[1;92m";  // Bold Light Green
        case LogCategory::AUDIO_MEDIA:       return "\033[1;96m";  // Bold Light Cyan
        case LogCategory::AUDIO_SYSTEM:      return "\033[1;93m";  // Bold Light Yellow
        case LogCategory::AUDIO_TELEPHONY:   return "\033[1;95m";  // Bold Light Magenta
        case LogCategory::AUDIO_MICROPHONE:  return "\033[1;91m";  // Bold Light Red
        
        // Video Categories - Magenta variants
        case LogCategory::VIDEO_SINK:        return "\033[1;95m";  // Bold Light Magenta
        case LogCategory::VIDEO_CHANNEL:     return "\033[95m";    // Light Magenta
        
        default: return "";
    }
}

std::string AasdkConsoleFormatter::resetColor() const {
    return useColors_ ? "\033[0m" : "";
}

// JsonFormatter Implementation
JsonFormatter::JsonFormatter(bool prettyPrint) : prettyPrint_(prettyPrint) {
}

std::string JsonFormatter::format(const LogEntry& entry) {
#ifdef ENABLE_JSON_LOGGING
    nlohmann::json json;
    
    // Convert timestamp to ISO 8601
    auto time_t = std::chrono::system_clock::to_time_t(entry.timestamp);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        entry.timestamp.time_since_epoch()) % 1000;
    
    std::ostringstream timeStr;
    timeStr << std::put_time(std::gmtime(&time_t), "%Y-%m-%dT%H:%M:%S");
    timeStr << "." << std::setfill('0') << std::setw(3) << ms.count() << "Z";
    
    json["timestamp"] = timeStr.str();
    json["level"] = ModernLogger::levelToString(entry.level);
    json["category"] = ModernLogger::categoryToString(entry.category);
    json["component"] = entry.component;
    json["function"] = entry.function;
    json["file"] = entry.file;
    json["line"] = entry.line;
    
    std::ostringstream threadStr;
    threadStr << entry.threadId;
    json["thread_id"] = threadStr.str();
    
    json["message"] = entry.message;
    
    if (!entry.context.empty()) {
        json["context"] = entry.context;
    }
    
    return json.dump(prettyPrint_ ? 2 : -1) + "\n";
#else
    // Fallback simple format when nlohmann/json is not available
    std::ostringstream oss;
    oss << "{\"level\":\"" << ModernLogger::levelToString(entry.level) 
        << "\",\"category\":\"" << ModernLogger::categoryToString(entry.category)
        << "\",\"message\":\"" << entry.message << "\"}\n";
    return oss.str();
#endif
}

// FileFormatter Implementation
FileFormatter::FileFormatter() {
}

std::string FileFormatter::format(const LogEntry& entry) {
    std::ostringstream oss;
    
    // Timestamp
    auto time_t = std::chrono::system_clock::to_time_t(entry.timestamp);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        entry.timestamp.time_since_epoch()) % 1000;
    
    oss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    oss << "." << std::setfill('0') << std::setw(3) << ms.count();
    
    // Level and category
    oss << " [" << ModernLogger::levelToString(entry.level) << "]";
    oss << " [" << ModernLogger::categoryToString(entry.category) << "]";
    
    // Thread ID
    oss << " [" << entry.threadId << "]";
    
    // Component and function
    oss << " [AASDK:" << entry.component << "::" << entry.function << "]";
    
    // Location
    if (!entry.file.empty()) {
        std::string filename = entry.file;
        size_t lastSlash = filename.find_last_of("/\\");
        if (lastSlash != std::string::npos) {
            filename = filename.substr(lastSlash + 1);
        }
        oss << " (" << filename << ":" << entry.line << ")";
    }
    
    // Message
    oss << " - " << entry.message;
    
    // Context
    if (!entry.context.empty()) {
        oss << " {";
        bool first = true;
        for (const auto& [key, value] : entry.context) {
            if (!first) oss << ", ";
            oss << key << "=" << value;
            first = false;
        }
        oss << "}";
    }
    
    oss << std::endl;
    return oss.str();
}

// ConsoleSink Implementation
ConsoleSink::ConsoleSink(bool useStderr) : useStderr_(useStderr) {
}

void ConsoleSink::write(const std::string& message) {
    if (useStderr_) {
        std::cerr << message;
    } else {
        std::cout << message;
    }
}

void ConsoleSink::flush() {
    if (useStderr_) {
        std::cerr.flush();
    } else {
        std::cout.flush();
    }
}

// FileSink Implementation
FileSink::FileSink(const std::string& filename, size_t maxSize, size_t maxFiles)
    : filename_(filename), maxSize_(maxSize), maxFiles_(maxFiles), currentSize_(0) {
    
    file_.open(filename_, std::ios::app);
    if (file_.is_open()) {
        file_.seekp(0, std::ios::end);
        currentSize_ = file_.tellp();
    }
}

FileSink::~FileSink() {
    if (file_.is_open()) {
        file_.close();
    }
}

void FileSink::write(const std::string& message) {
    std::lock_guard<std::mutex> lock(fileMutex_);
    
    if (!file_.is_open()) {
        return;
    }
    
    if (currentSize_ + message.size() > maxSize_) {
        rotateFile();
    }
    
    file_ << message;
    currentSize_ += message.size();
}

void FileSink::flush() {
    std::lock_guard<std::mutex> lock(fileMutex_);
    if (file_.is_open()) {
        file_.flush();
    }
}

void FileSink::rotateFile() {
    if (file_.is_open()) {
        file_.close();
    }
    
    // Rotate existing files
    for (size_t i = maxFiles_ - 1; i > 0; --i) {
        std::string oldFile = filename_ + "." + std::to_string(i);
        std::string newFile = filename_ + "." + std::to_string(i + 1);
        
        if (std::filesystem::exists(oldFile)) {
            if (i == maxFiles_ - 1) {
                std::filesystem::remove(newFile); // Remove oldest
            }
            std::filesystem::rename(oldFile, newFile);
        }
    }
    
    // Move current file to .1
    if (std::filesystem::exists(filename_)) {
        std::filesystem::rename(filename_, filename_ + ".1");
    }
    
    // Open new file
    file_.open(filename_, std::ios::out | std::ios::trunc);
    currentSize_ = 0;
}

// RemoteSink Implementation
RemoteSink::RemoteSink(const std::string& endpoint) : endpoint_(endpoint) {
}

void RemoteSink::write(const std::string& message) {
    std::lock_guard<std::mutex> lock(queueMutex_);
    pendingMessages_.push(message);
    
    // TODO: Implement actual remote sending (HTTP POST, TCP, etc.)
    // For now, just queue the messages
}

void RemoteSink::flush() {
    std::lock_guard<std::mutex> lock(queueMutex_);
    
    // TODO: Implement batch sending of pending messages
    while (!pendingMessages_.empty()) {
        // Send pendingMessages_.front() to endpoint_
        pendingMessages_.pop();
    }
}

} // namespace common
} // namespace aasdk
