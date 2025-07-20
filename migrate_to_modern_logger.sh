#!/bin/bash

# AASDK Modern Logger Migration Script with Channel Subcategories
# This script helps migrate existing AASDK_LOG calls to the new modern logger with detailed categories

echo "AASDK Modern Logger Migration Script (Enhanced)"
echo "==============================================="

AASDK_DIR="/home/pi/aasdk"

# Function to migrate files in a specific directory with a category
migrate_directory() {
    local dir=$1
    local category=$2
    echo "Migrating $dir to use $category category..."
    
    find "$AASDK_DIR/$dir" -name "*.cpp" -type f | while read -r file; do
        echo "  Processing: $file"
        
        # Replace simple debug logs
        sed -i 's/AASDK_LOG(debug) << "\[.*\] \(.*\)";/AASDK_LOG_'$category'(debug, "\1");/g' "$file"
        
        # Replace info logs
        sed -i 's/AASDK_LOG(info) << "\[.*\] \(.*\)";/AASDK_LOG_'$category'(info, "\1");/g' "$file"
        
        # Replace error logs
        sed -i 's/AASDK_LOG(error) << "\[.*\] \(.*\)";/AASDK_LOG_'$category'(error, "\1");/g' "$file"
        
        # Replace warn logs
        sed -i 's/AASDK_LOG(warn) << "\[.*\] \(.*\)";/AASDK_LOG_'$category'(warn, "\1");/g' "$file"
        
        # Add include for ModernLogger if not present
        if ! grep -q "ModernLogger.hpp" "$file"; then
            # Find the last #include line and add after it
            sed -i '/^#include.*Log\.hpp/a #include <aasdk/Common/ModernLogger.hpp>' "$file"
        fi
    done
}

# Function to migrate specific channel types with detailed subcategories
migrate_channel_subcategory() {
    local dir=$1
    local subcategory=$2
    echo "Migrating $dir to use CHANNEL_$subcategory subcategory..."
    
    find "$AASDK_DIR/$dir" -name "*.cpp" -type f | while read -r file; do
        echo "  Processing: $file"
        
        # Replace debug logs
        sed -i 's/AASDK_LOG(debug) << "\[.*\] \(.*\)";/AASDK_LOG_CHANNEL_'$subcategory'(debug, "\1");/g' "$file"
        
        # Replace info logs  
        sed -i 's/AASDK_LOG(info) << "\[.*\] \(.*\)";/AASDK_LOG_CHANNEL_'$subcategory'(info, "\1");/g' "$file"
        
        # Replace error logs
        sed -i 's/AASDK_LOG(error) << "\[.*\] \(.*\)";/AASDK_LOG_CHANNEL_'$subcategory'(error, "\1");/g' "$file"
        
        # Replace warn logs
        sed -i 's/AASDK_LOG(warn) << "\[.*\] \(.*\)";/AASDK_LOG_CHANNEL_'$subcategory'(warn, "\1");/g' "$file"
        
        # Add include for ModernLogger if not present
        if ! grep -q "ModernLogger.hpp" "$file"; then
            sed -i '/^#include.*Log\.hpp/a #include <aasdk/Common/ModernLogger.hpp>' "$file"
        fi
    done
}

# Function to migrate audio channels with specific audio subcategories
migrate_audio_subcategory() {
    local file_pattern=$1
    local subcategory=$2
    echo "Migrating $file_pattern to use AUDIO_$subcategory subcategory..."
    
    find "$AASDK_DIR" -name "$file_pattern" -type f | while read -r file; do
        echo "  Processing: $file"
        
        # Replace debug logs
        sed -i 's/AASDK_LOG(debug) << "\[.*\] \(.*\)";/AASDK_LOG_AUDIO_'$subcategory'(debug, "\1");/g' "$file"
        
        # Replace info logs
        sed -i 's/AASDK_LOG(info) << "\[.*\] \(.*\)";/AASDK_LOG_AUDIO_'$subcategory'(info, "\1");/g' "$file"
        
        # Replace error logs
        sed -i 's/AASDK_LOG(error) << "\[.*\] \(.*\)";/AASDK_LOG_AUDIO_'$subcategory'(error, "\1");/g' "$file"
        
        # Replace warn logs
        sed -i 's/AASDK_LOG(warn) << "\[.*\] \(.*\)";/AASDK_LOG_AUDIO_'$subcategory'(warn, "\1");/g' "$file"
        
        # Add include for ModernLogger if not present
        if ! grep -q "ModernLogger.hpp" "$file"; then
            sed -i '/^#include.*Log\.hpp/a #include <aasdk/Common/ModernLogger.hpp>' "$file"
        fi
    done
}

# Create backup
echo "Creating backup of src directory..."
cp -r "$AASDK_DIR/src" "$AASDK_DIR/src.backup.$(date +%Y%m%d_%H%M%S)"

# Migrate basic transport and infrastructure components
migrate_directory "src/Transport" "TRANSPORT"
migrate_directory "src/USB" "USB" 
migrate_directory "src/TCP" "TCP"
migrate_directory "src/Messenger" "MESSENGER"

# Migrate detailed channel subcategories
migrate_channel_subcategory "src/Channel/Control" "CONTROL"
migrate_channel_subcategory "src/Channel/Bluetooth" "BLUETOOTH"
migrate_channel_subcategory "src/Channel/InputSource" "INPUT_SOURCE"
migrate_channel_subcategory "src/Channel/SensorSource" "SENSOR_SOURCE"
migrate_channel_subcategory "src/Channel/NavigationStatus" "NAVIGATION"
migrate_channel_subcategory "src/Channel/PhoneStatus" "PHONE_STATUS"
migrate_channel_subcategory "src/Channel/Radio" "RADIO"
migrate_channel_subcategory "src/Channel/GenericNotification" "NOTIFICATION"
migrate_channel_subcategory "src/Channel/VendorExtension" "VENDOR_EXT"
migrate_channel_subcategory "src/Channel/WifiProjection" "WIFI_PROJECTION"
migrate_channel_subcategory "src/Channel/MediaBrowser" "MEDIA_BROWSER"
migrate_channel_subcategory "src/Channel/MediaPlaybackStatus" "PLAYBACK_STATUS"

# Migrate media sink and source
migrate_channel_subcategory "src/Channel/MediaSink" "MEDIA_SINK"
migrate_channel_subcategory "src/Channel/MediaSource" "MEDIA_SOURCE"

# Migrate specific audio channel types
migrate_audio_subcategory "*GuidanceAudioChannel.cpp" "GUIDANCE"
migrate_audio_subcategory "*MediaAudioChannel.cpp" "MEDIA"
migrate_audio_subcategory "*SystemAudioChannel.cpp" "SYSTEM"
migrate_audio_subcategory "*TelephonyAudioChannel.cpp" "TELEPHONY"
migrate_audio_subcategory "*MicrophoneAudioChannel.cpp" "MICROPHONE"

# Migrate video channels
find "$AASDK_DIR/src/Channel/MediaSink/Video" -name "*.cpp" -type f | while read -r file; do
    echo "  Processing video file: $file"
    sed -i 's/AASDK_LOG(debug) << "\[.*\] \(.*\)";/AASDK_LOG_VIDEO_SINK(debug, "\1");/g' "$file"
    sed -i 's/AASDK_LOG(info) << "\[.*\] \(.*\)";/AASDK_LOG_VIDEO_SINK(info, "\1");/g' "$file"
    sed -i 's/AASDK_LOG(error) << "\[.*\] \(.*\)";/AASDK_LOG_VIDEO_SINK(error, "\1");/g' "$file"
    sed -i 's/AASDK_LOG(warn) << "\[.*\] \(.*\)";/AASDK_LOG_VIDEO_SINK(warn, "\1");/g' "$file"
    
    if ! grep -q "ModernLogger.hpp" "$file"; then
        sed -i '/^#include.*Log\.hpp/a #include <aasdk/Common/ModernLogger.hpp>' "$file"
    fi
done

echo ""
echo "Enhanced migration completed!"
echo "Migrated the following categories:"
echo "  - Basic: TRANSPORT, USB, TCP, MESSENGER"
echo "  - Channel subcategories: CONTROL, BLUETOOTH, MEDIA_SINK, etc."
echo "  - Audio subcategories: GUIDANCE, MEDIA, SYSTEM, TELEPHONY, MICROPHONE"
echo "  - Video subcategories: VIDEO_SINK"
echo ""
echo "Please review the changes and test the build."
echo "Original files backed up to src.backup.*"
echo ""
echo "To build with modern logging:"
echo "  cd $AASDK_DIR/build"
echo "  make"
echo ""
echo "To configure detailed logging levels per category:"
echo "  auto& logger = aasdk::common::ModernLogger::getInstance();"
echo "  logger.setCategoryLevel(aasdk::common::LogCategory::CHANNEL_CONTROL, aasdk::common::LogLevel::TRACE);"
echo "  logger.setCategoryLevel(aasdk::common::LogCategory::AUDIO_MEDIA, aasdk::common::LogLevel::DEBUG);"
