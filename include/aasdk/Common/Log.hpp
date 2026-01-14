// This file is part of aasdk library project.
// Copyright (C) 2018 f1x.studio (Michal Szwaj)
// Copyright (C) 2024 CubeOne (Simon Dean - simon.dean@cubeone.co.uk)
// Copyright (C) 2025 OpenCarDev Team
// Copyright (C) 2026 OpenCarDev (Matthew Hilton - matthilton2005@gmail.com)
//
// aasdk is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// aasdk is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with aasdk. If not, see <http://www.gnu.org/licenses/>.

#pragma once

// Enable modern logging by default, but allow fallback to boost
#ifndef DISABLE_MODERN_LOGGING
#include <aasdk/Common/ModernLogger.hpp>
#include <sstream>

namespace aasdk { namespace common { namespace log_detail {
    // Helper class for stream-based logging compatible with existing code
    class LogStream {
    public:
        LogStream(const std::string& severity) {
            stream_ << "[AASDK] ";
            // Map boost log severity names to our LogLevel enum
            if (severity == "trace") level_ = aasdk::common::LogLevel::TRACE;
            else if (severity == "debug") level_ = aasdk::common::LogLevel::DEBUG;
            else if (severity == "info") level_ = aasdk::common::LogLevel::INFO;
            else if (severity == "warning" || severity == "warn") level_ = aasdk::common::LogLevel::WARN;
            else if (severity == "error") level_ = aasdk::common::LogLevel::ERROR;
            else if (severity == "fatal") level_ = aasdk::common::LogLevel::FATAL;
            else level_ = aasdk::common::LogLevel::INFO; // default
        }
        
        ~LogStream() {
            aasdk::common::ModernLogger::getInstance().log(
                level_,
                aasdk::common::LogCategory::GENERAL,
                "",
                "",
                "",
                0,
                stream_.str()
            );
        }
        
        template<typename T>
        LogStream& operator<<(const T& value) {
            stream_ << value;
            return *this;
        }
        
    private:
        std::ostringstream stream_;
        aasdk::common::LogLevel level_;
    };
}}}

// Modern logging macros with backward compatibility for stream-based usage
#define AASDK_LOG(severity) \
    aasdk::common::log_detail::LogStream(#severity)

// Category-specific logging macros for better organization
#define AASDK_LOG_CATEGORY(category, severity, message) \
    do { \
        std::ostringstream __stream; \
        __stream << "[AASDK] " << message; \
        aasdk::common::LogLevel __level; \
        if (std::string(#severity) == "trace") __level = aasdk::common::LogLevel::TRACE; \
        else if (std::string(#severity) == "debug") __level = aasdk::common::LogLevel::DEBUG; \
        else if (std::string(#severity) == "info") __level = aasdk::common::LogLevel::INFO; \
        else if (std::string(#severity) == "warning" || std::string(#severity) == "warn") __level = aasdk::common::LogLevel::WARN; \
        else if (std::string(#severity) == "error") __level = aasdk::common::LogLevel::ERROR; \
        else if (std::string(#severity) == "fatal") __level = aasdk::common::LogLevel::FATAL; \
        else __level = aasdk::common::LogLevel::INFO; \
        aasdk::common::ModernLogger::getInstance().log( \
            __level, \
            aasdk::common::LogCategory::category, \
            __PRETTY_FUNCTION__, \
            __FUNCTION__, \
            __FILE__, \
            __LINE__, \
            __stream.str()); \
    } while(0)

// Basic categories
#define AASDK_LOG_TRANSPORT(severity, message) AASDK_LOG_CATEGORY(TRANSPORT, severity, message)
#define AASDK_LOG_CHANNEL(severity, message) AASDK_LOG_CATEGORY(CHANNEL, severity, message) 
#define AASDK_LOG_USB(severity, message) AASDK_LOG_CATEGORY(USB, severity, message)
#define AASDK_LOG_TCP(severity, message) AASDK_LOG_CATEGORY(TCP, severity, message)
#define AASDK_LOG_MESSENGER(severity, message) AASDK_LOG_CATEGORY(MESSENGER, severity, message)
#define AASDK_LOG_PROTOCOL(severity, message) AASDK_LOG_CATEGORY(PROTOCOL, severity, message)
#define AASDK_LOG_AUDIO(severity, message) AASDK_LOG_CATEGORY(AUDIO, severity, message)
#define AASDK_LOG_VIDEO(severity, message) AASDK_LOG_CATEGORY(VIDEO, severity, message)
#define AASDK_LOG_INPUT(severity, message) AASDK_LOG_CATEGORY(INPUT, severity, message)
#define AASDK_LOG_SENSOR(severity, message) AASDK_LOG_CATEGORY(SENSOR, severity, message)
#define AASDK_LOG_BLUETOOTH(severity, message) AASDK_LOG_CATEGORY(BLUETOOTH, severity, message)
#define AASDK_LOG_WIFI(severity, message) AASDK_LOG_CATEGORY(WIFI, severity, message)
#define AASDK_LOG_SYSTEM(severity, message) AASDK_LOG_CATEGORY(SYSTEM, severity, message)
#define AASDK_LOG_GENERAL(severity, message) AASDK_LOG_CATEGORY(GENERAL, severity, message)

// Detailed channel categories
#define AASDK_LOG_CHANNEL_CONTROL(severity, message) AASDK_LOG_CATEGORY(CHANNEL_CONTROL, severity, message)
#define AASDK_LOG_CHANNEL_BLUETOOTH(severity, message) AASDK_LOG_CATEGORY(CHANNEL_BLUETOOTH, severity, message)
#define AASDK_LOG_CHANNEL_MEDIA_SINK(severity, message) AASDK_LOG_CATEGORY(CHANNEL_MEDIA_SINK, severity, message)
#define AASDK_LOG_CHANNEL_MEDIA_SOURCE(severity, message) AASDK_LOG_CATEGORY(CHANNEL_MEDIA_SOURCE, severity, message)
#define AASDK_LOG_CHANNEL_INPUT_SOURCE(severity, message) AASDK_LOG_CATEGORY(CHANNEL_INPUT_SOURCE, severity, message)
#define AASDK_LOG_CHANNEL_SENSOR_SOURCE(severity, message) AASDK_LOG_CATEGORY(CHANNEL_SENSOR_SOURCE, severity, message)
#define AASDK_LOG_CHANNEL_NAVIGATION(severity, message) AASDK_LOG_CATEGORY(CHANNEL_NAVIGATION, severity, message)
#define AASDK_LOG_CHANNEL_PHONE_STATUS(severity, message) AASDK_LOG_CATEGORY(CHANNEL_PHONE_STATUS, severity, message)
#define AASDK_LOG_CHANNEL_RADIO(severity, message) AASDK_LOG_CATEGORY(CHANNEL_RADIO, severity, message)
#define AASDK_LOG_CHANNEL_NOTIFICATION(severity, message) AASDK_LOG_CATEGORY(CHANNEL_NOTIFICATION, severity, message)
#define AASDK_LOG_CHANNEL_VENDOR_EXT(severity, message) AASDK_LOG_CATEGORY(CHANNEL_VENDOR_EXT, severity, message)
#define AASDK_LOG_CHANNEL_WIFI_PROJECTION(severity, message) AASDK_LOG_CATEGORY(CHANNEL_WIFI_PROJECTION, severity, message)
#define AASDK_LOG_CHANNEL_MEDIA_BROWSER(severity, message) AASDK_LOG_CATEGORY(CHANNEL_MEDIA_BROWSER, severity, message)
#define AASDK_LOG_CHANNEL_PLAYBACK_STATUS(severity, message) AASDK_LOG_CATEGORY(CHANNEL_PLAYBACK_STATUS, severity, message)

// Audio channel categories
#define AASDK_LOG_AUDIO_GUIDANCE(severity, message) AASDK_LOG_CATEGORY(AUDIO_GUIDANCE, severity, message)
#define AASDK_LOG_AUDIO_MEDIA(severity, message) AASDK_LOG_CATEGORY(AUDIO_MEDIA, severity, message)
#define AASDK_LOG_AUDIO_SYSTEM(severity, message) AASDK_LOG_CATEGORY(AUDIO_SYSTEM, severity, message)
#define AASDK_LOG_AUDIO_TELEPHONY(severity, message) AASDK_LOG_CATEGORY(AUDIO_TELEPHONY, severity, message)
#define AASDK_LOG_AUDIO_MICROPHONE(severity, message) AASDK_LOG_CATEGORY(AUDIO_MICROPHONE, severity, message)

// Video channel categories
#define AASDK_LOG_VIDEO_SINK(severity, message) AASDK_LOG_CATEGORY(VIDEO_SINK, severity, message)
#define AASDK_LOG_VIDEO_CHANNEL(severity, message) AASDK_LOG_CATEGORY(VIDEO_CHANNEL, severity, message)

#else
// Fallback to boost logging
#include <boost/log/trivial.hpp>
#define AASDK_LOG(severity) BOOST_LOG_TRIVIAL(severity) << "[AASDK] "
#endif
