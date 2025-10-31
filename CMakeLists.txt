cmake_minimum_required(VERSION 3.14)

project(aasdk)
message(STATUS "AASDK Library")
message(STATUS "Cross Compiling?")

# Cross Compiling Architecture and Package Architecture Detection
if( TARGET_ARCH STREQUAL "amd64" )
    message(STATUS "...amd64")
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
elseif( TARGET_ARCH STREQUAL "armhf" )
    message(STATUS "...armhf")
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "armhf")
    # Only set cross-compilation compilers if explicitly using cross-compilation
    # For native builds (e.g., Docker with QEMU), let CMake use default compilers
    if(DEFINED ENV{CMAKE_C_COMPILER} AND DEFINED ENV{CMAKE_CXX_COMPILER})
        # Environment variables are set (from build.sh), use them
        set(CMAKE_C_COMPILER $ENV{CMAKE_C_COMPILER})
        set(CMAKE_CXX_COMPILER $ENV{CMAKE_CXX_COMPILER})
    elseif(CMAKE_CROSSCOMPILING OR DEFINED CMAKE_TOOLCHAIN_FILE)
        # Only set cross-compilation when actually cross-compiling
        set(CMAKE_C_COMPILER /usr/bin/arm-linux-gnueabihf-gcc)
        set(CMAKE_CXX_COMPILER /usr/bin/arm-linux-gnueabihf-g++)
    endif()
elseif( TARGET_ARCH STREQUAL "arm64" )
    message(STATUS "...arm64")
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "arm64")
    # Only set cross-compilation compilers if explicitly using cross-compilation
    if(DEFINED ENV{CMAKE_C_COMPILER} AND DEFINED ENV{CMAKE_CXX_COMPILER})
        # Environment variables are set (from build.sh), use them
        set(CMAKE_C_COMPILER $ENV{CMAKE_C_COMPILER})
        set(CMAKE_CXX_COMPILER $ENV{CMAKE_CXX_COMPILER})
    elseif(CMAKE_CROSSCOMPILING OR DEFINED CMAKE_TOOLCHAIN_FILE)
        # Only set cross-compilation when actually cross-compiling
        set(CMAKE_C_COMPILER /usr/bin/aarch64-linux-gnu-gcc)
        set(CMAKE_CXX_COMPILER /usr/bin/aarch64-linux-gnu-g++)
    endif()
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

# Set CMAKE_MODULE_PATH early so we can include gitversion.cmake
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake_modules/")

# Get git commit information using gitversion.cmake module
set(local_dir ${CMAKE_CURRENT_SOURCE_DIR})
include(gitversion)

# Include DEB package filename generation module
include(DebPackageFilename)

# Clean up git commit ID (remove any unwanted characters)
if(NOT GIT_COMMIT_ID OR GIT_COMMIT_ID STREQUAL "")
    set(GIT_COMMIT_ID "unknown")
endif()
if(NOT GIT_BRANCH OR GIT_BRANCH STREQUAL "")
    set(GIT_BRANCH "unknown")
endif()
if(NOT GIT_DESCRIBE OR GIT_DESCRIBE STREQUAL "")
    set(GIT_DESCRIBE "unknown")
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
message(STATUS "Git Timestamp: ${GIT_COMMIT_TIMESTAMP}")
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
set(CPACK_PACKAGE_CONTACT "OpenCarDev Team <opencardevuk@gmail.com>")
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
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "AASDK <opencardevuk@gmail.com>")
set(CPACK_DEBIAN_PACKAGE_SECTION "libs")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/opencardev/aasdk")

# Compute runtime Depends automatically per distro/arch using dpkg-shlibdeps
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
# Ensure component runtime also gets shlibdeps
set(CPACK_DEBIAN_RUNTIME_PACKAGE_SHLIBDEPS ON)
# Help dpkg-shlibdeps find private libs in the build tree (if any)
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS_PRIVATE_DIRS "${CMAKE_CURRENT_BINARY_DIR}/lib;${CMAKE_CURRENT_BINARY_DIR}")
set(CPACK_DEBIAN_PACKAGE_RECOMMENDS "")
set(CPACK_DEBIAN_PACKAGE_SUGGESTS "")

# Configure DEB package filename using the DebPackageFilename module
configure_deb_filename(
    PACKAGE_NAME "${CPACK_PACKAGE_NAME}"
    VERSION "${LIBRARY_VERSION_STRING}"
    ARCHITECTURE "${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}"
    RUNTIME_PACKAGE_NAME "${CPACK_PACKAGE_NAME}"
    DEVELOPMENT_PACKAGE_NAME "${CPACK_PACKAGE_NAME}-dev"
)

# Component configuration for multi-package builds
set(CPACK_DEB_COMPONENT_INSTALL ON)
set(CPACK_COMPONENTS_ALL runtime development)

# Runtime package (libraries only)
set(CPACK_COMPONENT_RUNTIME_DISPLAY_NAME "AASDK Runtime Libraries for ${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}")
set(CPACK_COMPONENT_RUNTIME_DESCRIPTION "Runtime libraries for Android Auto SDK on ${CPACK_DEBIAN_PACKAGE_ARCHITECTURE} architecture")
set(CPACK_DEBIAN_RUNTIME_PACKAGE_NAME "${CPACK_PACKAGE_NAME}")
set(CPACK_DEBIAN_RUNTIME_PACKAGE_SECTION "libs")
set(CPACK_DEBIAN_RUNTIME_PACKAGE_PRIORITY "optional")
# Do not hardcode Depends here; let dpkg-shlibdeps compute per-distro/arch

# Development package (headers and cmake files)
set(CPACK_COMPONENT_DEVELOPMENT_DISPLAY_NAME "AASDK Development Files for ${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}")
set(CPACK_COMPONENT_DEVELOPMENT_DESCRIPTION "Development headers and CMake files for Android Auto SDK on ${CPACK_DEBIAN_PACKAGE_ARCHITECTURE} architecture")
set(CPACK_DEBIAN_DEVELOPMENT_PACKAGE_NAME "${CPACK_PACKAGE_NAME}-dev")
set(CPACK_DEBIAN_DEVELOPMENT_PACKAGE_SECTION "libdevel")
set(CPACK_DEBIAN_DEVELOPMENT_PACKAGE_PRIORITY "optional")
set(CPACK_DEBIAN_DEVELOPMENT_PACKAGE_DEPENDS "${CPACK_PACKAGE_NAME} (= ${LIBRARY_BUILD_MAJOR_RELEASE}.${LIBRARY_BUILD_MINOR_RELEASE}.${LIBRARY_BUILD_PATCH_RELEASE})")

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
