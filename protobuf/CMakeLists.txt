cmake_minimum_required(VERSION 3.16)

#Android Auto Projection Protocol Library
project(aap_protobuf)
message(STATUS "AASDK ProtoBuf Library")

# Use versioning from parent project if available, otherwise use defaults
if(DEFINED LIBRARY_BUILD_MAJOR_RELEASE AND DEFINED LIBRARY_BUILD_MINOR_RELEASE AND DEFINED LIBRARY_BUILD_PATCH_RELEASE)
    # Use parent project versioning
    set(AAP_VERSION_MAJOR ${LIBRARY_BUILD_MAJOR_RELEASE})
    set(AAP_VERSION_MINOR ${LIBRARY_BUILD_MINOR_RELEASE}) 
    set(AAP_VERSION_PATCH ${LIBRARY_BUILD_PATCH_RELEASE})
    
    # Use parent project's version string if available
    if(DEFINED LIBRARY_VERSION_STRING)
        set(LIBRARY_BUILD_VERSION_STRING ${LIBRARY_VERSION_STRING})
    else()
        set(LIBRARY_BUILD_VERSION_STRING "${AAP_VERSION_MAJOR}.${AAP_VERSION_MINOR}.${AAP_VERSION_PATCH}")
    endif()
else()
    # Fallback to original versioning for standalone builds
    set(LIBRARY_BUILD_DATE "20241121")    # Binary Release Build Date
    set(LIBRARY_BUILD_MAJOR_RELEASE 4)        # Binary Release Build Number (increment if released on same day)
    set(LIBRARY_BUILD_MINOR_RELEASE 0)        # Binary Release Build Number (increment if released on same day)
    set(LIBRARY_BUILD_INCREMENTAL 0)    # Binary Build Version - Increment for Each Build
    set(LIBRARY_BUILD_VERSION_STRING "${LIBRARY_BUILD_MAJOR_RELEASE}.${LIBRARY_BUILD_MINOR_RELEASE}.${LIBRARY_BUILD_INCREMENTAL}+${LIBRARY_BUILD_DATE}")
endif()
set(AAP_PROTO_VERSION 1.6)          # Underlying AAP (Android Auto Projection Library Protocol)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Default to Release mode unless overridden with -DCMAKE_BUILD_TYPE=Debug on cmake command
if(NOT CMAKE_BUILD_TYPE)
    message(STATUS "Forcing Release build type")
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
    # Set the possible values of build type for cmake-gui
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

# Use FetchContent to build Protobuf v30.0 with C++20 support
message(STATUS "AASDK will build and package Google Protobuf v30.0")
include(FetchContent)

# Fetch Abseil (required by Protobuf v22.0+)
FetchContent_Declare(
  abseil
  GIT_REPOSITORY https://github.com/abseil/abseil-cpp.git
  GIT_TAG 20240722.0  # LTS 2024-07-22, compatible with Protobuf v30.0
  GIT_SHALLOW TRUE
)

# Save current compiler flags to restore later
set(SAVED_CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

# Remove -pedantic from flags for external dependencies (Abseil, Protobuf)
string(REPLACE " -pedantic" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

set(ABSL_PROPAGATE_CXX_STD ON CACHE BOOL "" FORCE)
set(ABSL_BUILD_TESTING OFF CACHE BOOL "" FORCE)
message(STATUS "Fetching Abseil LTS 2024-07-22...")
FetchContent_MakeAvailable(abseil)

# Fetch Protobuf v30.0
FetchContent_Declare(
  protobuf
  GIT_REPOSITORY https://github.com/protocolbuffers/protobuf.git
  GIT_TAG v30.0
  GIT_SHALLOW TRUE
)
set(protobuf_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(protobuf_ABSL_PROVIDER "package" CACHE STRING "" FORCE)
set(protobuf_BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)
set(protobuf_INSTALL ON CACHE BOOL "" FORCE)
message(STATUS "Fetching Protobuf v30.0...")

# Make protobuf available - this will configure and build it
FetchContent_MakeAvailable(protobuf)

# Restore original compiler flags for AASDK code
set(CMAKE_CXX_FLAGS "${SAVED_CMAKE_CXX_FLAGS}")
message(STATUS "Restored original compiler flags")

# Set protobuf_BIN_DIR to use the built protoc from FetchContent
set(PROTOBUF_PROTOC_EXECUTABLE "${protobuf_BINARY_DIR}/protoc")
message(STATUS "Using protoc from build: ${PROTOBUF_PROTOC_EXECUTABLE}")

# Include protobuf module to get protobuf_generate function
list(APPEND CMAKE_MODULE_PATH ${protobuf_SOURCE_DIR}/cmake)
find_package(Protobuf REQUIRED CONFIG)

# Locate all proto files
file(GLOB_RECURSE PROTO_FILES ${CMAKE_CURRENT_SOURCE_DIR}/aap_protobuf/*.proto)

# Create the library first with proto files as sources
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")  # macOS
    message(NOTICE "Configuring STATIC Library for MacOS")
    add_library(aap_protobuf STATIC ${PROTO_FILES})
else()
    message(NOTICE "Configuring SHARED Library")
    add_library(aap_protobuf SHARED ${PROTO_FILES})
endif()

# Use the modern protobuf_generate function
protobuf_generate(
    TARGET aap_protobuf
    IMPORT_DIRS ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/aap_protobuf
)

# Link against the fetched protobuf library (PRIVATE to avoid export issues)
target_link_libraries(aap_protobuf PRIVATE protobuf::libprotobuf)
target_include_directories(aap_protobuf PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/aap_protobuf>
    $<BUILD_INTERFACE:${protobuf_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:include/aap_protobuf>
)

set(LIBRARY_BUILD_VERSION_STRING "${LIBRARY_BUILD_MAJOR_RELEASE}.${LIBRARY_BUILD_MINOR_RELEASE}.${LIBRARY_BUILD_INCREMENTAL}+${LIBRARY_BUILD_DATE}")
message(STATUS "aap_protobuf Project Version: ${LIBRARY_BUILD_VERSION_STRING}")

# Set version properties
if(DEFINED LIBRARY_BUILD_MAJOR_RELEASE)
    set_target_properties(aap_protobuf
            PROPERTIES
            VERSION ${LIBRARY_BUILD_VERSION_STRING}
            SOVERSION ${LIBRARY_BUILD_MAJOR_RELEASE})
else()
    set_target_properties(aap_protobuf
            PROPERTIES
            VERSION ${LIBRARY_BUILD_VERSION_STRING}
            SOVERSION 4)
endif()

# Install rules with component assignment for packaging
install(TARGETS aap_protobuf
        EXPORT aap_protobufTargets
        LIBRARY DESTINATION lib COMPONENT runtime
        ARCHIVE DESTINATION lib COMPONENT runtime
        RUNTIME DESTINATION bin COMPONENT runtime
        INCLUDES DESTINATION include
)

target_include_directories(aap_protobuf
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<INSTALL_INTERFACE:include/aap_protobuf>
)

# Install headers explicitly with component assignment
install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/aap_protobuf
        DESTINATION include
        COMPONENT development
        FILES_MATCHING PATTERN "*.h")

# Export the targets to a script
install(EXPORT aap_protobufTargets
        FILE aap_protobufTargets.cmake
        NAMESPACE AAPProto::
        DESTINATION lib/cmake/aap_protobuf
        COMPONENT development
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
        DESTINATION lib/cmake/aap_protobuf
        COMPONENT development)

# Install Protobuf v30.0 libraries and tools built by FetchContent
message(STATUS "Installing Protobuf v30.0 runtime and development files")
install(TARGETS protobuf protoc
        RUNTIME DESTINATION bin COMPONENT runtime
        LIBRARY DESTINATION lib COMPONENT runtime
        ARCHIVE DESTINATION lib COMPONENT development
)

# Install protobuf headers
install(DIRECTORY ${protobuf_SOURCE_DIR}/src/google
        DESTINATION include
        COMPONENT development
        FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
        PATTERN "*/test" EXCLUDE
)

# Install protobuf cmake config files
install(DIRECTORY ${protobuf_BINARY_DIR}/cmake
        DESTINATION lib/cmake/protobuf
        COMPONENT development
        FILES_MATCHING PATTERN "*.cmake"
)

# CPack packaging (DEB)
set(CPACK_GENERATOR "DEB")
set(CPACK_PACKAGE_NAME "aap-protobuf")
set(CPACK_PACKAGE_VENDOR "OpenCarDev Team")
set(CPACK_PACKAGE_CONTACT "OpenCarDev Team")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "AASDK Protobuf library")
set(CPACK_PACKAGE_VERSION "${LIBRARY_BUILD_VERSION_STRING}")
set(CPACK_DEBIAN_PACKAGE_SECTION "libs")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_RUNTIME_PACKAGE_SHLIBDEPS ON)

if(NOT DEFINED CPACK_DEBIAN_PACKAGE_ARCHITECTURE)
    execute_process(
        COMMAND dpkg --print-architecture
        OUTPUT_VARIABLE _dpkg_arch
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    if(_dpkg_arch)
        set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "${_dpkg_arch}")
    else()
        set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
    endif()
endif()

set(CPACK_DEB_COMPONENT_INSTALL ON)
set(CPACK_COMPONENTS_ALL runtime development)
set(CPACK_DEBIAN_RUNTIME_PACKAGE_NAME "aap-protobuf")
set(CPACK_DEBIAN_DEVELOPMENT_PACKAGE_NAME "aap-protobuf-dev")
set(CPACK_COMPONENT_DEVELOPMENT_DEPENDS runtime)
set(CPACK_DEBIAN_DEVELOPMENT_PACKAGE_DEPENDS "aap-protobuf (= ${CPACK_PACKAGE_VERSION})")

include(CPack)


