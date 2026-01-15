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
message(STATUS "Using FetchContent to download and build Protobuf v30.0")
include(FetchContent)

# Fetch Abseil (required by Protobuf v22.0+)
FetchContent_Declare(
  abseil
  GIT_REPOSITORY https://github.com/abseil/abseil-cpp.git
  GIT_TAG 20240722.0  # LTS 2024-07-22, compatible with Protobuf v30.0
  GIT_SHALLOW TRUE
)
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
  SOURCE_SUBDIR cmake
)
set(protobuf_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(protobuf_ABSL_PROVIDER "package" CACHE STRING "" FORCE)
set(protobuf_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
message(STATUS "Fetching Protobuf v30.0...")
FetchContent_MakeAvailable(protobuf)

# Get the protobuf module path from the fetched content
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${protobuf_SOURCE_DIR}/cmake)

# Use the fetched protobuf's protoc compiler
set(PROTOBUF_PROTOC_EXECUTABLE $<TARGET_FILE:protobuf::protoc>)

# Locate all proto files
file(GLOB_RECURSE source_proto_files ${CMAKE_CURRENT_SOURCE_DIR}/*.proto)

# Compile ProtoBuf files using the fetched protobuf
foreach(proto_file ${source_proto_files})
    get_filename_component(proto_file_abs ${proto_file} ABSOLUTE)
    get_filename_component(proto_dir ${proto_file_abs} DIRECTORY)
    get_filename_component(proto_name ${proto_file_abs} NAME_WE)
    
    set(proto_src "${CMAKE_CURRENT_BINARY_DIR}/aap_protobuf/${proto_name}.pb.cc")
    set(proto_hdr "${CMAKE_CURRENT_BINARY_DIR}/aap_protobuf/${proto_name}.pb.h")
    
    list(APPEND PROTO_SRCS ${proto_src})
    list(APPEND PROTO_HDRS ${proto_hdr})
    
    add_custom_command(
        OUTPUT ${proto_src} ${proto_hdr}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/aap_protobuf
        COMMAND protobuf::protoc
        ARGS --cpp_out=${CMAKE_CURRENT_BINARY_DIR}/aap_protobuf
             --proto_path=${proto_dir}
             ${proto_file_abs}
        DEPENDS ${proto_file_abs} protobuf::protoc
        COMMENT "Running C++ protobuf compiler on ${proto_file}"
        VERBATIM
    )
endforeach()

# Generate Library
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")  # macOS
    message(NOTICE "Configuring STATIC Library for MacOS")
    add_library(aap_protobuf STATIC ${PROTO_SRCS} ${PROTO_HDRS})
else()
    message(NOTICE "Configuring SHARED Library")
    add_library(aap_protobuf SHARED ${PROTO_SRCS} ${PROTO_HDRS})
endif()

# Link against the fetched protobuf library
target_link_libraries(aap_protobuf PUBLIC protobuf::libprotobuf)
target_include_directories(aap_protobuf PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/aap_protobuf>
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


