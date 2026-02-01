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
  GIT_REPOSITORY https://github.com/protocolbuffers/protobuf.git
  GIT_TAG v30.0
  GIT_SHALLOW TRUE
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

# When called as a standalone project (e.g., from workflow), only package protobuf
# When called as a submodule, also build AAP protocol definitions
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    # Standalone build - only package protobuf v30.0 with Abseil dependencies
    message(STATUS "Building in standalone mode - packaging Google Protobuf v30.0 with Abseil")
    
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
else()
    # Submodule mode - also build AAP protocol definitions
    message(STATUS "Building as submodule - also compiling AAP protocol definitions")

    # Use protobuf_generate from the CMake module (it's available after FetchContent_MakeAvailable)
    # The function is defined in cmake/protobuf-generate.cmake which gets included automatically
    include(${protobuf_SOURCE_DIR}/cmake/protobuf-generate.cmake)
    
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

    # Link against the fetched protobuf library (PUBLIC so consumers get protobuf dependency)
    target_link_libraries(aap_protobuf PUBLIC protobuf::libprotobuf)
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
        
    # End of submodule mode section
    # CPack will package both aap_protobuf library and protobuf dependency
endif()

# CPack packaging (DEB) - applies to both standalone and submodule modes
set(CPACK_GENERATOR "DEB")
set(CPACK_PACKAGE_NAME "aap-protobuf")
set(CPACK_PACKAGE_VENDOR "OpenCarDev Team")
set(CPACK_PACKAGE_CONTACT "OpenCarDev Team")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "AASDK Protobuf library and Google Protobuf v30.0")
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
    
    # Disable automatic dpkg-shlibdeps dependency generation. We ship Abseil
    # shared libraries inside this package (private libs) and dpkg-shlibdeps
    # fails to resolve them in this packaging context. We provide conservative
    # manual runtime dependencies instead.
    set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS OFF)
    set(CPACK_DEBIAN_RUNTIME_PACKAGE_DEPENDS "libc6 (>= 2.34)")
    
    include(CPack)

# Install Protobuf v30.0 libraries and tools built by FetchContent
message(STATUS "Installing Protobuf v30.0 runtime and development files")
install(TARGETS libprotobuf protoc
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


