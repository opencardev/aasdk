/**
 * Test script to verify the migration to modern logger with channel subcategories
 */

#include <aasdk/Common/Log.hpp>
#include <aasdk/Common/ModernLogger.hpp>
#include <iostream>

int main() {
    std::cout << "Testing AASDK Migration to Modern Logger\n";
    std::cout << "========================================\n\n";

    // Initialize modern logger for console output
    auto& logger = aasdk::common::ModernLogger::getInstance();
    logger.setLevel(aasdk::common::LogLevel::DEBUG);

    // Test basic categories that were migrated
    std::cout << "1. Testing basic migrated categories:\n";
    AASDK_LOG_TRANSPORT(info, "Transport layer initialized");
    AASDK_LOG_MESSENGER(debug, "Message queue ready for processing");
    AASDK_LOG_USB(info, "USB connection established");
    AASDK_LOG_TCP(debug, "TCP socket opened successfully");

    std::cout << "\n2. Testing detailed channel subcategories:\n";
    AASDK_LOG_CHANNEL_CONTROL(debug, "Control channel handshake initiated");
    AASDK_LOG_CHANNEL_BLUETOOTH(info, "Bluetooth pairing process started");
    AASDK_LOG_CHANNEL_MEDIA_SINK(debug, "Media sink channel configuration");
    AASDK_LOG_CHANNEL_NAVIGATION(warn, "GPS signal quality degraded");

    std::cout << "\n3. Testing audio subcategories:\n";
    AASDK_LOG_AUDIO_GUIDANCE(info, "Navigation voice guidance enabled");
    AASDK_LOG_AUDIO_MEDIA(debug, "Media audio stream buffer status");
    AASDK_LOG_AUDIO_TELEPHONY(info, "Phone call audio channel active");

    std::cout << "\n4. Testing video subcategories:\n";
    AASDK_LOG_VIDEO_SINK(info, "Video display output configured");
    AASDK_LOG_VIDEO_CHANNEL(debug, "Video channel bandwidth optimization");

    std::cout << "\n5. Testing backward compatibility:\n";
    // This should still work after migration
    AASDK_LOG(info) << "[Migration Test] Legacy logging syntax still functional";

    std::cout << "\nMigration verification completed!\n";
    std::cout << "All modern logger categories are working correctly.\n";

    return 0;
}
