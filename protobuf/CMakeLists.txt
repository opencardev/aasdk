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
message(STATUS "Building Google Protobuf v30.0 as standalone dependency")
include(FetchContent)

# Fetch Abseil (required by Protobuf v22.0+)
FetchContent_Declare(
  abseil
  URL https://github.com/abseil/abseil-cpp/archive/refs/tags/20240722.0.zip
)

# Ensure external static libs are built with -fPIC for shared protobuf
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Save current compiler flags to restore later
set(SAVED_CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

# Remove -pedantic from flags for external dependencies (Abseil, Protobuf)
string(REPLACE " -pedantic" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

# Build Abseil as static libraries to avoid linking issues
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(ABSL_PROPAGATE_CXX_STD ON CACHE BOOL "" FORCE)
set(ABSL_BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(ABSL_ENABLE_INSTALL ON CACHE BOOL "" FORCE)
message(STATUS "Fetching Abseil LTS 2024-07-22...")
FetchContent_MakeAvailable(abseil)

# Fetch Protobuf v30.0
FetchContent_Declare(
  protobuf
  URL https://github.com/protocolbuffers/protobuf/archive/refs/tags/v30.0.zip
)
set(protobuf_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(protobuf_ABSL_PROVIDER "package" CACHE STRING "" FORCE)
set(protobuf_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(protobuf_INSTALL ON CACHE BOOL "" FORCE)
message(STATUS "Fetching Protobuf v30.0...")

# Make protobuf available - this will configure and build it
FetchContent_MakeAvailable(protobuf)

# Restore original compiler flags for AASDK code
set(CMAKE_CXX_FLAGS "${SAVED_CMAKE_CXX_FLAGS}")
message(STATUS "Restored original compiler flags")

# Protobuf target is now available - configure CMake to find it
set(Protobuf_PROTOC_EXECUTABLE "${protobuf_BINARY_DIR}/protoc")
message(STATUS "Protobuf targets available, protoc: ${Protobuf_PROTOC_EXECUTABLE}")

# This is always a standalone build now - build and install protobuf
message(STATUS "Building standalone protobuf installation")

# Install protobuf runtime and development files
install(TARGETS libprotobuf libprotobuf-lite libprotoc protoc
    RUNTIME DESTINATION bin COMPONENT runtime
    LIBRARY DESTINATION lib COMPONENT runtime
        ARCHIVE DESTINATION lib COMPONENT development
    )
    
    # Install Abseil shared libraries that protobuf depends on
    # List all required Abseil libraries explicitly
    set(ABSL_LIBS_TO_INSTALL
        absl_log_initialize
        absl_log_internal_check_op
        absl_log_internal_conditions
        absl_log_internal_message
        absl_log_internal_nullguard
        absl_log_internal_log_sink_set
        absl_die_if_null
        absl_statusor
        absl_status
        absl_raw_hash_set
        absl_hashtablez_sampler
        absl_hash
        absl_city
        absl_low_level_hash
        absl_cord
        absl_cordz_info
        absl_cord_internal
        absl_cordz_functions
        absl_exponential_biased
        absl_cordz_handle
        absl_crc_cord_state
        absl_crc32c
        absl_crc_internal
        absl_crc_cpu_detect
        absl_synchronization
        absl_graphcycles_internal
        absl_kernel_timeout_internal
        absl_time
        absl_time_zone
        absl_civil_time
        absl_str_format_internal
        absl_strings
        absl_strings_internal
        absl_string_view
        absl_int128
        absl_spinlock_wait
        absl_throw_delegate
        absl_stacktrace
        absl_symbolize
        absl_debugging_internal
        absl_demangle_internal
        absl_malloc_internal
        absl_base
        absl_raw_logging_internal
        absl_log_severity
        utf8_validity
        utf8_range
    )
    
    # Install Abseil libraries - only needed for shared library builds
    # For static builds, Abseil is linked statically into protobuf
    if(BUILD_SHARED_LIBS)
        # Install Abseil shared libraries by copying built .so files from the
        # Abseil build tree. This avoids relying on exported target names which
        # may not be available in this directory scope and ensures dpkg-shlibdeps
        # can find the libraries when packaging.
        file(GLOB_RECURSE ABSEIL_SHARED_LIBS
            RELATIVE "${abseil_BINARY_DIR}"
            "${abseil_BINARY_DIR}/**/libabsl_*.so*"
            "${abseil_BINARY_DIR}/**/libutf8_*.so*"
        )
        if(ABSEIL_SHARED_LIBS)
            # Determine Debian multi-arch library directory when possible so
            # dpkg-shlibdeps will locate shared libraries referenced by RPATHs.
            set(ABSEIL_LIB_INSTALL_DIR lib)
            if(DEFINED CPACK_DEBIAN_PACKAGE_ARCHITECTURE AND CPACK_DEBIAN_PACKAGE_ARCHITECTURE STREQUAL "amd64")
                set(ABSEIL_LIB_INSTALL_DIR lib/x86_64-linux-gnu)
            elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
                set(ABSEIL_LIB_INSTALL_DIR lib/x86_64-linux-gnu)
            endif()

            foreach(lib ${ABSEIL_SHARED_LIBS})
                install(FILES "${abseil_BINARY_DIR}/${lib}"
                    DESTINATION ${ABSEIL_LIB_INSTALL_DIR}
                    COMPONENT runtime
                )
                # Collect installed libs for shlibs generation
                list(APPEND _INSTALLED_ABSEIL_LIBS "${abseil_BINARY_DIR}/${lib}")
            endforeach()
        else()
            message(WARNING "No Abseil shared libraries found under ${abseil_BINARY_DIR}; packaging may fail")
        endif()
    else()
        message(STATUS "Building static libraries - Abseil will be linked statically into protobuf")
    endif()

    # Generate a minimal shlibs.local file from the installed Abseil shared libs.
    # This helps dpkg-shlibdeps resolve private libraries when packaging.
    # Only needed for shared library builds
    if(BUILD_SHARED_LIBS AND _INSTALLED_ABSEIL_LIBS)
        set(SHLIBS_FILE "${CMAKE_CURRENT_BINARY_DIR}/shlibs.local")
        file(WRITE "${SHLIBS_FILE}" "# Local shlibs for aap-protobuf\n")
        foreach(libpath IN LISTS _INSTALLED_ABSEIL_LIBS)
            if(EXISTS "${libpath}")
                execute_process(
                    COMMAND objdump -p "${libpath}"
                    OUTPUT_VARIABLE _objdump_out
                    ERROR_QUIET
                )
                string(REGEX MATCH "SONAME[ \\t]*([^ \\n\\r]+)" _match "${_objdump_out}")
                if(_match)
                    string(REGEX REPLACE "SONAME[ \\t]*" "" _soname_line "${_match}")
                    string(STRIP "${_soname_line}" _soname)
                    # Extract major soname version if present (e.g., libfoo.so.1)
                    string(REGEX MATCH "(.*\\.so\\.[0-9]+)" _so_and_ver "${_soname}")
                    if(_so_and_ver)
                        set(_soname_key "${_so_and_ver}")
                    else()
                        set(_soname_key "${_soname}")
                    endif()
                    # Write a conservative mapping to the local shlibs file
                    file(APPEND "${SHLIBS_FILE}" "${_soname_key} aap-protobuf (>= ${LIBRARY_BUILD_VERSION_STRING})\n")
                endif()
            endif()
        endforeach()

        # Install the shlibs.local into the package so dpkg-shlibdeps can consult it
        install(FILES "${SHLIBS_FILE}"
            DESTINATION lib/pkgconfig
            COMPONENT development
        )
    endif()  # BUILD_SHARED_LIBS AND _INSTALLED_ABSEIL_LIBS
    
    # Install protobuf headers
    install(DIRECTORY ${protobuf_SOURCE_DIR}/src/google
            DESTINATION include
            COMPONENT development
            FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
            PATTERN "*test*" EXCLUDE
            PATTERN "*benchmark*" EXCLUDE
    )
    
    # Install Abseil headers
    install(DIRECTORY ${abseil_SOURCE_DIR}/absl
            DESTINATION include
            COMPONENT development
            FILES_MATCHING PATTERN "*.h" PATTERN "*.inc"
            PATTERN "*test*" EXCLUDE
            PATTERN "*benchmark*" EXCLUDE
    )
    
    # Install protobuf cmake config files
    install(DIRECTORY ${protobuf_BINARY_DIR}/
            DESTINATION lib/cmake/protobuf
            COMPONENT development
            FILES_MATCHING PATTERN "protobuf*.cmake"
            PATTERN "CMakeFiles" EXCLUDE
            PATTERN "*.dir" EXCLUDE
    )
    
    # Install Abseil cmake config files
    install(DIRECTORY ${abseil_BINARY_DIR}/
            DESTINATION lib/cmake/absl
            COMPONENT development
            FILES_MATCHING PATTERN "absl*.cmake"
            PATTERN "CMakeFiles" EXCLUDE
            PATTERN "*.dir" EXCLUDE
    )
    
    # Generate and install pkg-config file for protobuf
    set(PROTOBUF_PC_VERSION "${LIBRARY_BUILD_VERSION_STRING}")
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/protobuf.pc.in
        ${CMAKE_CURRENT_BINARY_DIR}/protobuf.pc
        @ONLY
    )
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/protobuf.pc
            DESTINATION lib/pkgconfig
            COMPONENT development
    )


