cmake_minimum_required(VERSION 3.14)

project(aasdk)
message(STATUS "AASDK Library")
message(STATUS "Cross Compiling?")

# Cross Compiling Architecture
if( TARGET_ARCH STREQUAL "amd64" )
    message(STATUS "...amd64")
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
elseif( TARGET_ARCH STREQUAL "armhf" )
    message(STATUS "...armhf")
    set(CMAKE_C_COMPILER /usr/bin/arm-linux-gnueabihf-gcc-8)
    set(CMAKE_CXX_COMPILER /usr/bin/arm-linux-gnueabihf-g++-8)
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "armhf")
elseif( TARGET_ARCH STREQUAL "arm64" )
    message(STATUS "...arm64")
    set(CMAKE_C_COMPILER /usr/bin/aarch64-linux-gnu-gcc)
    set(CMAKE_CXX_COMPILER /usr/bin/aarch64-linux-gnu-g++)
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "arm64")
elseif( TARGET_ARCH STREQUAL "i386" )
    message(STATUS "...i386")
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "i386")
else()
    message(STATUS "...not cross compiling")
    # Auto-detect architecture for native builds
    execute_process(
        COMMAND dpkg --print-architecture
        OUTPUT_VARIABLE DETECTED_ARCH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    if(DETECTED_ARCH)
        set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "${DETECTED_ARCH}")
        message(STATUS "Detected architecture: ${DETECTED_ARCH}")
    else()
        # Fallback to system processor
        if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
            set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
        elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
            set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "arm64")
        elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "armv7l")
            set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "armhf")
        elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "i386|i686")
            set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "i386")
        else()
            set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "unknown")
        endif()
        message(STATUS "Fallback architecture detection: ${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}")
    endif()
endif()

# Set Compile Versions - Date-based semantic versioning
string(TIMESTAMP LIBRARY_BUILD_YEAR "%Y" UTC)      # Major version = Year (e.g., 2025)
string(TIMESTAMP LIBRARY_BUILD_MONTH "%m" UTC)     # Minor version = Month (01-12)
string(TIMESTAMP LIBRARY_BUILD_DAY "%d" UTC)       # Patch version = Day (01-31)

# Get git commit information
find_package(Git QUIET)
if(GIT_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_COMMIT_ID
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_BRANCH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    execute_process(
        COMMAND ${GIT_EXECUTABLE} describe --tags --dirty --always
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_DESCRIBE
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    # Check if repository is dirty (has uncommitted changes)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} diff --quiet
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        RESULT_VARIABLE GIT_DIRTY
        ERROR_QUIET
    )
else()
    set(GIT_COMMIT_ID "unknown")
    set(GIT_BRANCH "unknown")
    set(GIT_DESCRIBE "unknown")
    set(GIT_DIRTY 0)
endif()

# Clean up git commit ID (remove any unwanted characters)
if(NOT GIT_COMMIT_ID)
    set(GIT_COMMIT_ID "unknown")
endif()

# Set version components
set(LIBRARY_BUILD_MAJOR_RELEASE ${LIBRARY_BUILD_YEAR})
set(LIBRARY_BUILD_MINOR_RELEASE ${LIBRARY_BUILD_MONTH})
set(LIBRARY_BUILD_PATCH_RELEASE ${LIBRARY_BUILD_DAY})

# Cache
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
endif()

set(base_directory ${CMAKE_CURRENT_SOURCE_DIR})
set(sources_directory ${base_directory}/src)

set(include_directory ${base_directory}/include)
set(include_ut_directory ${base_directory}/unit_test)

# Configure CMAKE
message(STATUS "Configuring CMAKE")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake_modules/")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS_INIT} -fPIC -Wall -pedantic")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "-g -O3 -DNDEBUG")

# Default to Release mode unless overridden with -DCMAKE_BUILD_TYPE=Debug on cmake command
if(NOT CMAKE_BUILD_TYPE)
    message(STATUS "Forcing Release build type")
    set(CMAKE_BUILD_TYPE Release)
endif()

# Modern Logger options
option(DISABLE_MODERN_LOGGING "Disable modern logging and use boost fallback" OFF)
option(ENABLE_JSON_LOGGING "Enable JSON formatter support (requires nlohmann/json)" OFF)

if(DISABLE_MODERN_LOGGING)
    message(STATUS "Modern logging disabled - using boost fallback")
    add_definitions(-DDISABLE_MODERN_LOGGING)
else()
    message(STATUS "Modern logging enabled")
    if(ENABLE_JSON_LOGGING)
        message(STATUS "JSON logging support enabled")
        add_definitions(-DENABLE_JSON_LOGGING)
        find_package(nlohmann_json QUIET)
        if(nlohmann_json_FOUND)
            message(STATUS "Found nlohmann/json")
        else()
            message(WARNING "nlohmann/json not found - JSON logging will use fallback format")
        endif()
    endif()
endif()

# Paths
set(sources_directory ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(include_directory ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(include_ut_directory ${CMAKE_CURRENT_SOURCE_DIR}/include_ut)

# Set output directories to build directory for proper packaging
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/bin)

set (CMAKE_PROJECT_VERSION_PATCH ${_commit_timestamp})

# Configure Boost
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)

add_definitions(-DBOOST_ALL_DYN_LINK)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    message(STATUS "Disabling Boost DEBUG logs")
    add_definitions(-DNDEBUG)
endif()

if(WIN32)
    set(WINSOCK2_LIBRARIES "ws2_32")
endif(WIN32)

if(AASDK_TEST)
    include(FetchContent)
    FetchContent_Declare(
            googletest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG main
    )
    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
endif(AASDK_TEST)

# Build aap_protobuf as part of this project
message(STATUS "Building aap_protobuf as subdirectory")
add_subdirectory(protobuf)

# Building on a Mac requires Abseil
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")  # macOS
    message(STATUS "MacOS System Detected")
    find_package(protobuf REQUIRED CONFIG)
    find_package(absl REQUIRED)
else ()
    find_package(Protobuf REQUIRED)
endif ()

include(FindProtobuf)

find_package(Boost REQUIRED COMPONENTS system log_setup log OPTIONAL_COMPONENTS unit_test_framework)

# Capture Boost version for packaging
if(Boost_VERSION)
    string(REPLACE "." ";" VERSION_LIST ${Boost_VERSION})
    list(GET VERSION_LIST 0 Boost_VERSION_MAJOR)
    list(GET VERSION_LIST 1 Boost_VERSION_MINOR)
    set(BOOST_PACKAGE_VERSION "${Boost_VERSION_MAJOR}.${Boost_VERSION_MINOR}.0")
    message(STATUS "Detected Boost version: ${BOOST_PACKAGE_VERSION}")
else()
    # Fallback to common versions if detection fails
    set(BOOST_PACKAGE_VERSION "1.83.0")
    message(WARNING "Could not detect Boost version, using fallback: ${BOOST_PACKAGE_VERSION}")
endif()

find_package(libusb-1.0 REQUIRED)
find_package(OpenSSL REQUIRED)

include_directories(
        ${Boost_INCLUDE_DIRS}
        ${PROTOBUF_INCLUDE_DIR}
        ${OPENSSL_INCLUDE_DIR}
        ${LIBUSB_1_INCLUDE_DIRS}
        ${GTEST_INCLUDE_DIRS}
        ${GMOCK_INCLUDE_DIRS}
        ${include_directory}
        ${include_ut_directory})

file(GLOB_RECURSE source_files ${sources_directory}/*.cpp)
file(GLOB_RECURSE include_files ${include_directory}/*.hpp)
file(GLOB_RECURSE tests_source_files ${sources_directory}/*.ut.cpp)
file(GLOB_RECURSE tests_include_files ${include_ut_directory}/*.hpp)

list(REMOVE_ITEM source_files ${tests_source_files})

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")  # macOS
    message(NOTICE "Configuring STATIC Library for MacOS")
    add_library(aasdk STATIC
            ${source_files}
            ${include_files})
else()
    message(NOTICE "Configuring SHARED Library")
    add_library(aasdk SHARED
            ${source_files}
            ${include_files})
endif()
target_include_directories(aasdk PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/protobuf>
        $<INSTALL_INTERFACE:include/aap_protobuf>
)

target_link_libraries(aasdk PUBLIC
        ${LIBUSB_1_LIBRARIES}
        ${Boost_LIBRARIES}
        ${PROTOBUF_LIBRARIES}
        aap_protobuf
        ${OPENSSL_LIBRARIES}
        ${WINSOCK2_LIBRARIES})

# Link nlohmann/json if available and JSON logging is enabled
if(ENABLE_JSON_LOGGING AND nlohmann_json_FOUND)
    target_link_libraries(aasdk PUBLIC nlohmann_json::nlohmann_json)
endif()


set(LIBRARY_VERSION_STRING "${LIBRARY_BUILD_MAJOR_RELEASE}.${LIBRARY_BUILD_MINOR_RELEASE}.${LIBRARY_BUILD_PATCH_RELEASE}")

# Add git commit info to version if available
if(GIT_COMMIT_ID AND NOT GIT_COMMIT_ID STREQUAL "unknown")
    set(LIBRARY_VERSION_STRING "${LIBRARY_VERSION_STRING}+git.${GIT_COMMIT_ID}")
    if(GIT_DIRTY GREATER 0)
        set(LIBRARY_VERSION_STRING "${LIBRARY_VERSION_STRING}.dirty")
    endif()
endif()

# Add build type suffix for debug builds
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(LIBRARY_VERSION_STRING "${LIBRARY_VERSION_STRING}.debug")
endif()

message(STATUS "Project Version: ${LIBRARY_VERSION_STRING}")
message(STATUS "Git Branch: ${GIT_BRANCH}")
message(STATUS "Git Commit: ${GIT_COMMIT_ID}")
message(STATUS "Git Describe: ${GIT_DESCRIBE}")
if(GIT_DIRTY GREATER 0)
    message(STATUS "Repository Status: DIRTY (has uncommitted changes)")
else()
    message(STATUS "Repository Status: CLEAN")
endif()

# Generate version header file
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/include/aasdk/Version.hpp.in"
    "${CMAKE_CURRENT_BINARY_DIR}/include/aasdk/Version.hpp"
    @ONLY
)

# Add the binary dir to include path for generated headers
include_directories("${CMAKE_CURRENT_BINARY_DIR}/include")

set_target_properties(aasdk
        PROPERTIES VERSION ${LIBRARY_VERSION_STRING} SOVERSION ${LIBRARY_BUILD_MAJOR_RELEASE})

# Install rules
install(TARGETS aasdk
        EXPORT aasdkTargets
        LIBRARY DESTINATION lib COMPONENT runtime
        ARCHIVE DESTINATION lib COMPONENT runtime
        RUNTIME DESTINATION bin COMPONENT runtime
        INCLUDES DESTINATION include
)

# Install headers explicitly
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/aasdk
        DESTINATION include
        COMPONENT development
)

# Install generated version header
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/include/aasdk/Version.hpp"
        DESTINATION include/aasdk
        COMPONENT development
)

# Export the targets to a script
install(EXPORT aasdkTargets
        FILE aasdkTargets.cmake
        NAMESPACE AASDK::
        DESTINATION lib/cmake/aasdk
        COMPONENT development
)

# Create a Config file for FindPackage
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/aasdkConfigVersion.cmake"
        VERSION 1.0
        COMPATIBILITY AnyNewerVersion
)

configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/Config.cmake.in
        "${CMAKE_CURRENT_BINARY_DIR}/aasdkConfig.cmake"
        INSTALL_DESTINATION lib/cmake/aasdk
)

# Install the config files
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/aasdkConfigVersion.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/aasdkConfig.cmake"
        DESTINATION lib/cmake/aasdk
        COMPONENT development)

if(AASDK_TEST)
    add_executable(aasdk_ut
            ${tests_source_files}
            ${tests_include_files})

    add_dependencies(aasdk_ut aasdk)
    target_link_libraries(aasdk_ut
            aasdk
            gtest_main
            gmock_main)

    if(AASDK_CODE_COVERAGE)
        include(CodeCoverage)
        append_coverage_compiler_flags()
        setup_target_for_coverage(NAME aasdk_coverage EXECUTABLE aasdk_ut DEPENDENCIES aasdk_ut)
    endif(AASDK_CODE_COVERAGE)
endif(AASDK_TEST)


# CPack Configuration for DEB packages
set(CPACK_GENERATOR "DEB")
set(CPACK_PACKAGE_NAME "libaasdk")
set(CPACK_PACKAGE_VENDOR "AASDK")
set(CPACK_PACKAGE_VERSION ${LIBRARY_VERSION_STRING})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Android Auto SDK Library")
set(CPACK_PACKAGE_DESCRIPTION "Android Auto SDK (AASDK) is a library for implementing Android Auto functionality in C++ applications.")
set(CPACK_PACKAGE_CONTACT "AASDK <aasdk@example.com>")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/opencardev/aasdk")

# Set the correct install prefix for packaging
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set(CMAKE_INSTALL_PREFIX "/usr/local" CACHE PATH "Default install prefix" FORCE)
endif()

# Debian package configuration
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "AASDK <matthilton2005@gmail.com>")
set(CPACK_DEBIAN_PACKAGE_SECTION "libs")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/opencardev/aasdk")

# Build dynamic dependencies string with detected Boost version
set(BOOST_SYSTEM_DEP "libboost-system${BOOST_PACKAGE_VERSION}")
set(BOOST_LOG_DEP "libboost-log${BOOST_PACKAGE_VERSION}")
# Add fallback for common versions to ensure compatibility
if(BOOST_PACKAGE_VERSION VERSION_EQUAL "1.83.0")
    set(BOOST_SYSTEM_FALLBACK " | libboost-system1.81.0 | libboost-system1.74.0")
    set(BOOST_LOG_FALLBACK " | libboost-log1.81.0 | libboost-log1.74.0")
elseif(BOOST_PACKAGE_VERSION VERSION_EQUAL "1.81.0")
    set(BOOST_SYSTEM_FALLBACK " | libboost-system1.74.0 | libboost-system1.83.0")
    set(BOOST_LOG_FALLBACK " | libboost-log1.74.0 | libboost-log1.83.0")
else()
    set(BOOST_SYSTEM_FALLBACK " | libboost-system1.83.0 | libboost-system1.81.0 | libboost-system1.74.0")
    set(BOOST_LOG_FALLBACK " | libboost-log1.83.0 | libboost-log1.81.0 | libboost-log1.74.0")
endif()

set(CPACK_DEBIAN_PACKAGE_DEPENDS "libusb-1.0-0 (>= 1.0.0), ${BOOST_SYSTEM_DEP}${BOOST_SYSTEM_FALLBACK}, ${BOOST_LOG_DEP}${BOOST_LOG_FALLBACK}, libssl3 (>= 3.0.0), libprotobuf32 (>= 3.21.0)")
set(CPACK_DEBIAN_PACKAGE_RECOMMENDS "")
set(CPACK_DEBIAN_PACKAGE_SUGGESTS "")

# Configure different packages based on build type and architecture
set(ARCH_SUFFIX "-${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}")

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CPACK_PACKAGE_NAME "libaasdk${ARCH_SUFFIX}-dbg")
    set(CPACK_DEBIAN_PACKAGE_NAME "libaasdk${ARCH_SUFFIX}-dbg")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Android Auto SDK Library (Debug) for ${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}")
    set(CPACK_PACKAGE_DESCRIPTION "Android Auto SDK (AASDK) debug library with debugging symbols and information for ${CPACK_DEBIAN_PACKAGE_ARCHITECTURE} architecture.")
    set(CPACK_DEBIAN_PACKAGE_SECTION "debug")
    set(CPACK_DEBIAN_PACKAGE_PRIORITY "extra")
else()
    set(CPACK_PACKAGE_NAME "libaasdk${ARCH_SUFFIX}")
    set(CPACK_DEBIAN_PACKAGE_NAME "libaasdk${ARCH_SUFFIX}")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Android Auto SDK Library for ${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}")
    set(CPACK_PACKAGE_DESCRIPTION "Android Auto SDK (AASDK) is a library for implementing Android Auto functionality in C++ applications for ${CPACK_DEBIAN_PACKAGE_ARCHITECTURE} architecture.")
endif()

# File naming based on build type and architecture
if(DEFINED CPACK_DEBIAN_PACKAGE_ARCHITECTURE)
    set(CPACK_DEBIAN_FILE_NAME "${CPACK_DEBIAN_PACKAGE_NAME}_${LIBRARY_VERSION_STRING}_${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}.deb")
else()
    set(CPACK_DEBIAN_FILE_NAME "DEB-DEFAULT")
endif()

# Component configuration for multi-package builds
set(CPACK_DEB_COMPONENT_INSTALL ON)
set(CPACK_COMPONENTS_ALL runtime development)

# Runtime package (libraries only)
set(CPACK_COMPONENT_RUNTIME_DISPLAY_NAME "AASDK Runtime Libraries for ${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}")
set(CPACK_COMPONENT_RUNTIME_DESCRIPTION "Runtime libraries for Android Auto SDK on ${CPACK_DEBIAN_PACKAGE_ARCHITECTURE} architecture")
set(CPACK_DEBIAN_RUNTIME_PACKAGE_NAME "${CPACK_DEBIAN_PACKAGE_NAME}")
set(CPACK_DEBIAN_RUNTIME_FILE_NAME "${CPACK_DEBIAN_PACKAGE_NAME}_${LIBRARY_VERSION_STRING}_${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}.deb")
set(CPACK_DEBIAN_RUNTIME_PACKAGE_SECTION "libs")
set(CPACK_DEBIAN_RUNTIME_PACKAGE_PRIORITY "optional")
set(CPACK_DEBIAN_RUNTIME_PACKAGE_DEPENDS "${CPACK_DEBIAN_PACKAGE_DEPENDS}")

# Development package (headers and cmake files)
set(CPACK_COMPONENT_DEVELOPMENT_DISPLAY_NAME "AASDK Development Files for ${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}")
set(CPACK_COMPONENT_DEVELOPMENT_DESCRIPTION "Development headers and CMake files for Android Auto SDK on ${CPACK_DEBIAN_PACKAGE_ARCHITECTURE} architecture")
set(CPACK_DEBIAN_DEVELOPMENT_PACKAGE_NAME "${CPACK_DEBIAN_PACKAGE_NAME}-dev")
set(CPACK_DEBIAN_DEVELOPMENT_FILE_NAME "${CPACK_DEBIAN_PACKAGE_NAME}-dev_${LIBRARY_VERSION_STRING}_${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}.deb")
set(CPACK_DEBIAN_DEVELOPMENT_PACKAGE_SECTION "libdevel")
set(CPACK_DEBIAN_DEVELOPMENT_PACKAGE_PRIORITY "optional")
set(CPACK_DEBIAN_DEVELOPMENT_PACKAGE_DEPENDS "${CPACK_DEBIAN_PACKAGE_NAME} (= ${LIBRARY_VERSION_STRING})")

# Package relationships
set(CPACK_COMPONENT_DEVELOPMENT_DEPENDS runtime)

# Post-install and pre-removal scripts (if they exist)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/debian/postinst" AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/debian/prerm")
    # For component-based packaging, set control scripts for each component
    set(CPACK_DEBIAN_RUNTIME_PACKAGE_CONTROL_EXTRA "${CMAKE_CURRENT_SOURCE_DIR}/debian/postinst;${CMAKE_CURRENT_SOURCE_DIR}/debian/prerm")
    set(CPACK_DEBIAN_DEVELOPMENT_PACKAGE_CONTROL_EXTRA "${CMAKE_CURRENT_SOURCE_DIR}/debian/postinst;${CMAKE_CURRENT_SOURCE_DIR}/debian/prerm")
    # Also set the global one for backwards compatibility
    set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_CURRENT_SOURCE_DIR}/debian/postinst;${CMAKE_CURRENT_SOURCE_DIR}/debian/prerm")
endif()

# Source package configuration
set(CPACK_SOURCE_GENERATOR "TGZ;TBZ2")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${LIBRARY_VERSION_STRING}-src")
set(CPACK_SOURCE_IGNORE_FILES
    "/build/"
    "/build-debug/"
    "/[.]git/"
    "/[.]vscode/"
    "/[.]idea/"
    "[.]gitignore"
    "[.]DS_Store"
    "/test_build/"
    "/lib/"
    "/bin/"
    "[.]so$"
    "[.]a$"
)

include(CPack)
