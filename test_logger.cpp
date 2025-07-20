/*
*  Test program for AASDK Modern Logger with Channel Subcategories
*/

#include <aasdk/Common/ModernLogger.hpp>
#include <aasdk/Common/Log.hpp>
#include <iostream>

using namespace aasdk::common;

int main() {
    std::cout << "Testing AASDK Modern Logger with Channel Subcategories" << std::endl;
    std::cout << "======================================================" << std::endl;

    // Configure logger for detailed channel logging
    auto& logger = ModernLogger::getInstance();
    logger.setLevel(LogLevel::DEBUG);
    
    // Set different levels for different channel categories
    logger.setCategoryLevel(LogCategory::CHANNEL_CONTROL, LogLevel::TRACE);
    logger.setCategoryLevel(LogCategory::AUDIO_MEDIA, LogLevel::DEBUG);
    logger.setCategoryLevel(LogCategory::VIDEO_SINK, LogLevel::INFO);

    std::cout << "\n1. Testing basic categories:" << std::endl;
    AASDK_LOG_TRANSPORT(info, "Transport subsystem initialized");
    AASDK_LOG_USB(debug, "USB device connected");
    
    std::cout << "\n2. Testing detailed channel subcategories:" << std::endl;
    AASDK_LOG_CHANNEL_CONTROL(debug, "Control channel handshake initiated");
    AASDK_LOG_CHANNEL_BLUETOOTH(info, "Bluetooth pairing successful");
    AASDK_LOG_CHANNEL_MEDIA_SINK(debug, "Media sink channel opened");
    AASDK_LOG_CHANNEL_MEDIA_SOURCE(info, "Media source ready");
    AASDK_LOG_CHANNEL_INPUT_SOURCE(debug, "Input source configured");
    AASDK_LOG_CHANNEL_SENSOR_SOURCE(info, "Sensor data stream active");
    AASDK_LOG_CHANNEL_NAVIGATION(warn, "GPS signal weak");
    AASDK_LOG_CHANNEL_PHONE_STATUS(info, "Phone connected");
    AASDK_LOG_CHANNEL_RADIO(debug, "Radio frequency set");
    AASDK_LOG_CHANNEL_NOTIFICATION(info, "Notification received");
    AASDK_LOG_CHANNEL_VENDOR_EXT(debug, "Vendor extension loaded");
    AASDK_LOG_CHANNEL_WIFI_PROJECTION(info, "WiFi projection enabled");
    AASDK_LOG_CHANNEL_MEDIA_BROWSER(debug, "Media browser initialized");
    AASDK_LOG_CHANNEL_PLAYBACK_STATUS(info, "Playback status updated");
    
    std::cout << "\n3. Testing audio channel subcategories:" << std::endl;
    AASDK_LOG_AUDIO_GUIDANCE(info, "Navigation voice guidance ready");
    AASDK_LOG_AUDIO_MEDIA(debug, "Media audio stream started");
    AASDK_LOG_AUDIO_SYSTEM(warn, "System audio volume adjusted");
    AASDK_LOG_AUDIO_TELEPHONY(info, "Phone call audio active");
    AASDK_LOG_AUDIO_MICROPHONE(debug, "Microphone input detected");
    
    std::cout << "\n4. Testing video channel subcategories:" << std::endl;
    AASDK_LOG_VIDEO_SINK(info, "Video sink ready for display");
    AASDK_LOG_VIDEO_CHANNEL(debug, "Video channel configuration complete");
    
    std::cout << "\n5. Testing backward compatibility:" << std::endl;
    AASDK_LOG(info) << "Legacy logging still works perfectly";
    AASDK_LOG(debug) << "Debug with variable: " << 123;
    
    std::cout << "\n6. Testing context logging with channels:" << std::endl;
    std::map<std::string, std::string> context;
    context["channel_id"] = "0x42";
    context["bandwidth"] = "1920x1080@60fps";
    context["codec"] = "H.264";
    
    logger.logWithContext(
        LogLevel::INFO,
        LogCategory::VIDEO_SINK,
        "VideoSinkService",
        __FUNCTION__,
        __FILE__,
        __LINE__,
        "Video stream established with optimal settings",
        context
    );
    
    std::cout << "\n7. Demonstrating category-specific log levels:" << std::endl;
    
    // This should appear (CHANNEL_CONTROL is set to TRACE)
    AASDK_LOG_CHANNEL_CONTROL(trace, "Detailed control protocol trace");
    
    // This should appear (AUDIO_MEDIA is set to DEBUG)  
    AASDK_LOG_AUDIO_MEDIA(debug, "Audio buffer status check");
    
    // This should appear (VIDEO_SINK is set to INFO)
    AASDK_LOG_VIDEO_SINK(info, "Video frame rendered");
    
    // This might not appear depending on global level
    AASDK_LOG_VIDEO_SINK(debug, "This debug message might be filtered");

    logger.flush();
    
    std::cout << "\nChannel subcategory testing completed!" << std::endl;
    std::cout << "Check the detailed, categorized log output above." << std::endl;
    
    return 0;
}
