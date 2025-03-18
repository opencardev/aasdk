cmake_minimum_required(VERSION 3.5)

#Android Auto Projection Protocol Library
project(aap_protobuf)
message(STATUS "AASDK ProtoBuf Library")

# Set Compile Versions
set(LIBRARY_BUILD_DATE "20241121")    # Binary Release Build Date
set(LIBRARY_BUILD_MAJOR_RELEASE 4)        # Binary Release Build Number (increment if released on same day)
set(LIBRARY_BUILD_MINOR_RELEASE 0)        # Binary Release Build Number (increment if released on same day)
set(LIBRARY_BUILD_INCREMENTAL 0)    # Binary Build Version - Increment for Each Build
set(AAP_PROTO_VERSION 1.6)          # Underlying AAP (Android Auto Projection Library Protocol)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Default to Release mode unless overridden with -DCMAKE_BUILD_TYPE=Debug on cmake command
if(NOT CMAKE_BUILD_TYPE)
    message(STATUS "Forcing Release build type")
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
    # Set the possible values of build type for cmake-gui
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

# Building on a Mac requires Abseil
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")  # macOS
    # macOS specific command
    message(STATUS "MacOS System Detected")
    find_package(protobuf REQUIRED CONFIG)
    find_package(absl)
else()
    # Linux specific command
    find_package(Protobuf REQUIRED)
endif()

include(FindProtobuf)

# Turn off Path Appending
set(PROTOBUF_GENERATE_CPP_APPEND_PATH OFF)

# Locate all proto files
file(GLOB_RECURSE source_proto_files ${CMAKE_CURRENT_SOURCE_DIR}/*.proto)

# Compile ProtoBuf files
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${source_proto_files})

# Generate Library
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")  # macOS
    message(NOTICE "Configuring STATIC Library for MacOS")
    add_library(aap_protobuf STATIC ${PROTO_SRCS} ${PROTO_HDRS})
else()
    message(NOTICE "Configuring SHARED Library")
    add_library(aap_protobuf SHARED ${PROTO_SRCS} ${PROTO_HDRS})
endif()

target_link_libraries(aap_protobuf PUBLIC ${PROTOBUF_LIBRARIES})
include_directories(
        ${CMAKE_CURRENT_BINARY_DIR}
        ${CMAKE_CURRENT_BINARY_DIR}/aap_protobuf
        ${PROTOBUF_INCLUDE_DIR}
)

set(LIBRARY_BUILD_VERSION_STRING "${LIBRARY_BUILD_MAJOR_RELEASE}.${LIBRARY_BUILD_MINOR_RELEASE}.${LIBRARY_BUILD_INCREMENTAL}+${LIBRARY_BUILD_DATE}")
message(STATUS "Project Version: ${LIBRARY_BUILD_VERSION_STRING}")
set_target_properties(aap_protobuf
        PROPERTIES
        VERSION ${LIBRARY_BUILD_VERSION_STRING}
        SOVERSION ${LIBRARY_BUILD_INCREMENTAL})

# Install rules
install(TARGETS aap_protobuf
        EXPORT aap_protobufTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
)

target_include_directories(aap_protobuf
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<INSTALL_INTERFACE:include/aap_protobuf>
)

# Install headers explicitly
install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/aap_protobuf
        DESTINATION include
        FILES_MATCHING PATTERN "*.h")

# Export the targets to a script
install(EXPORT aap_protobufTargets
        FILE aap_protobufTargets.cmake
        NAMESPACE AAPProto::
        DESTINATION lib/cmake/aap_protobuf
)

# Create a Config file for FindPackage
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/aap_protobufConfigVersion.cmake"
        VERSION 1.0
        COMPATIBILITY AnyNewerVersion
)

configure_package_config_file(Config.cmake.in
        "${CMAKE_CURRENT_BINARY_DIR}/aap_protobufConfig.cmake"
        INSTALL_DESTINATION lib/cmake/aap_protobuf
)

# Install the config files
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/aap_protobufConfigVersion.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/aap_protobufConfig.cmake"
        DESTINATION lib/cmake/aap_protobuf)


