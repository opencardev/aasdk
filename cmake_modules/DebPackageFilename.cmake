#  * Project: OpenAuto
#  * This file is part of openauto project.
#  * Copyright (C) 2025 OpenCarDev Team
#  *
#  *  openauto is free software: you can redistribute it and/or modify
#  *  it under the terms of the GNU General Public License as published by
#  *  the Free Software Foundation; either version 3 of the License, or
#  *  (at your option) any later version.
#  *
#  *  openauto is distributed in the hope that it will be useful,
#  *  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  *  GNU General Public License for more details.
#  *
#  *  You should have received a copy of the GNU General Public License
#  *  along with openauto. If not, see <http://www.gnu.org/licenses/>.

# DebPackageFilename.cmake
# Generates Debian package filename based on distribution and architecture
#
# This module sets the following variables:
#   DEB_SUITE       - Distribution codename (e.g., bookworm, trixie, jammy)
#   DEB_VERSION     - Distribution version (e.g., deb12, deb13, ubuntu22.04)
#   DEB_RELEASE     - Release string for filename (e.g., deb13u1, 0ubuntu1~22.04)
#
# Usage:
#   include(DebPackageFilename)
#   configure_deb_filename(PACKAGE_NAME <name> VERSION <version> ARCHITECTURE <arch>)
message(STATUS "------- Start DebPackageFilename------")
message(STATUS "PACKAGE_NAME: ${DEB_CONFIG_PACKAGE_NAME}")
message(STATUS "VERSION: ${DEB_CONFIG_VERSION}")
message(STATUS "ARCHITECTURE: ${DEB_CONFIG_ARCHITECTURE}")
message(STATUS "-------End DebPackageFilename------")

function(get_linux_lsb_release_information)
    find_program(LSB_RELEASE_EXEC lsb_release)
    if(NOT LSB_RELEASE_EXEC)
        message(FATAL_ERROR "Could not detect lsb_release executable, can not gather required information")
    endif()

    execute_process(COMMAND "${LSB_RELEASE_EXEC}" --short --id OUTPUT_VARIABLE LSB_RELEASE_ID_SHORT OUTPUT_STRIP_TRAILING_WHITESPACE)
    execute_process(COMMAND "${LSB_RELEASE_EXEC}" --short --release OUTPUT_VARIABLE LSB_RELEASE_VERSION_SHORT OUTPUT_STRIP_TRAILING_WHITESPACE)
    execute_process(COMMAND "${LSB_RELEASE_EXEC}" --short --codename OUTPUT_VARIABLE LSB_RELEASE_CODENAME_SHORT OUTPUT_STRIP_TRAILING_WHITESPACE)

    set(LSB_RELEASE_ID_SHORT "${LSB_RELEASE_ID_SHORT}" PARENT_SCOPE)
    set(LSB_RELEASE_VERSION_SHORT "${LSB_RELEASE_VERSION_SHORT}" PARENT_SCOPE)
    set(LSB_RELEASE_CODENAME_SHORT "${LSB_RELEASE_CODENAME_SHORT}" PARENT_SCOPE)
endfunction()

# Detect suite/codename for filename
function(detect_deb_suite OUT_SUITE)
    message(STATUS "detect_deb_suite")
    set(DETECTED_SUITE "unknown")
    
    # 1. Check environment variable override
    if(DEFINED ENV{DISTRO_DEB_SUITE} AND NOT "$ENV{DISTRO_DEB_SUITE}" STREQUAL "")
        message(STATUS "Using DISTRO_DEB_SUITE: $ENV{DISTRO_DEB_SUITE}")
        set(DETECTED_SUITE "$ENV{DISTRO_DEB_SUITE}")
    # 2. Try to detect from lsb_release directly
    elseif(CMAKE_SYSTEM_NAME MATCHES "Linux")
        find_program(LSB_RELEASE_EXEC lsb_release)
        if(LSB_RELEASE_EXEC)
            execute_process(COMMAND "${LSB_RELEASE_EXEC}" --short --codename OUTPUT_VARIABLE LSB_CODENAME OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
            if(LSB_CODENAME AND NOT LSB_CODENAME STREQUAL "")
                set(DETECTED_SUITE "${LSB_CODENAME}")
                message(STATUS "Detected suite from lsb_release: ${LSB_CODENAME}")
            endif()
        endif()
    endif()
    
    message(STATUS "Detected Suite: ${DETECTED_SUITE}")
    set(${OUT_SUITE} "${DETECTED_SUITE}" PARENT_SCOPE)
endfunction()

# Detect debian version for filename (e.g. deb13, deb12, ubuntu24.04)
function(detect_deb_version OUT_VERSION)
    message(STATUS "detect_deb_version")
    set(DETECTED_VERSION "unknown")
    
    # Try environment variable first
    if(DEFINED ENV{DISTRO_DEB_VERSION} AND NOT "$ENV{DISTRO_DEB_VERSION}" STREQUAL "")
        set(DETECTED_VERSION "$ENV{DISTRO_DEB_VERSION}")
    # Try to detect from lsb_release
    elseif(CMAKE_SYSTEM_NAME MATCHES "Linux")
        find_program(LSB_RELEASE_EXEC lsb_release)
        if(LSB_RELEASE_EXEC)
            execute_process(COMMAND "${LSB_RELEASE_EXEC}" --short --id OUTPUT_VARIABLE LSB_ID OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
            execute_process(COMMAND "${LSB_RELEASE_EXEC}" --short --release OUTPUT_VARIABLE LSB_RELEASE OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
            
            if(LSB_ID AND LSB_RELEASE)
                if(LSB_ID STREQUAL "Ubuntu")
                    # Ubuntu format: ubuntu24.04
                    set(DETECTED_VERSION "ubuntu${LSB_RELEASE}")
                    message(STATUS "Detected Ubuntu version: ${DETECTED_VERSION}")
                elseif(LSB_RELEASE MATCHES "^([0-9]+)")
                    # Debian format: deb13, deb12, etc.
                    set(DETECTED_VERSION "deb${CMAKE_MATCH_1}")
                    message(STATUS "Detected Debian version: ${DETECTED_VERSION}")
                endif()
            endif()
        endif()
    endif()
    
    message(STATUS "DETECTED_VERSION: ${DETECTED_VERSION}")
    set(${OUT_VERSION} "${DETECTED_VERSION}" PARENT_SCOPE)
endfunction()

# Extract release string for filename
function(detect_deb_release OUT_RELEASE)
    message(STATUS "detect_deb_release")
    set(DETECTED_RELEASE "unknown")
    
    # Try environment variable first
    if(DEFINED ENV{DISTRO_DEB_RELEASE} AND NOT "$ENV{DISTRO_DEB_RELEASE}" STREQUAL "")
        set(DETECTED_RELEASE "$ENV{DISTRO_DEB_RELEASE}")
    # Use CPACK_DEBIAN_PACKAGE_RELEASE if available
    elseif(DEFINED CPACK_DEBIAN_PACKAGE_RELEASE AND NOT "${CPACK_DEBIAN_PACKAGE_RELEASE}" STREQUAL "")
        # Remove any leading version numbers (e.g. 1+deb13u1 -> deb13u1)
        string(REGEX REPLACE "^[0-9]+[\+~]?" "" DETECTED_RELEASE "${CPACK_DEBIAN_PACKAGE_RELEASE}")
    # Fallback: use detected version as release identifier
    else()
        # Try to construct a sensible release identifier
        if(CMAKE_SYSTEM_NAME MATCHES "Linux")
            find_program(LSB_RELEASE_EXEC lsb_release)
            if(LSB_RELEASE_EXEC)
                execute_process(COMMAND "${LSB_RELEASE_EXEC}" --short --id OUTPUT_VARIABLE LSB_ID OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
                execute_process(COMMAND "${LSB_RELEASE_EXEC}" --short --release OUTPUT_VARIABLE LSB_RELEASE OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
                
                if(LSB_ID STREQUAL "Ubuntu")
                    set(DETECTED_RELEASE "0ubuntu1~${LSB_RELEASE}")
                    message(STATUS "Generated Ubuntu release string: ${DETECTED_RELEASE}")
                elseif(LSB_RELEASE MATCHES "^([0-9]+)")
                    set(DETECTED_RELEASE "deb${CMAKE_MATCH_1}")
                    message(STATUS "Generated Debian release string: ${DETECTED_RELEASE}")
                endif()
            endif()
        endif()
    endif()
    
    message(STATUS "DETECTED_RELEASE: ${DETECTED_RELEASE}")
    set(${OUT_RELEASE} "${DETECTED_RELEASE}" PARENT_SCOPE)
endfunction()

# Configure the DEB package filename
function(configure_deb_filename)
    cmake_parse_arguments(
        DEB_CONFIG
        ""
        "PACKAGE_NAME;VERSION;ARCHITECTURE;RUNTIME_PACKAGE_NAME;DEVELOPMENT_PACKAGE_NAME"
        ""
        ${ARGN}
    )
    
    if(NOT DEFINED DEB_CONFIG_PACKAGE_NAME)
        message(FATAL_ERROR "PACKAGE_NAME is required for configure_deb_filename")
    endif()
    
    if(NOT DEFINED DEB_CONFIG_VERSION)
        message(FATAL_ERROR "VERSION is required for configure_deb_filename")
    endif()
    
    if(NOT DEFINED DEB_CONFIG_ARCHITECTURE)
        message(FATAL_ERROR "ARCHITECTURE is required for configure_deb_filename")
    endif()
    
    # Detect all components
    detect_deb_suite(DEB_SUITE_VAL)
    detect_deb_version(DEB_VERSION_VAL)
    detect_deb_release(DEB_RELEASE_VAL)
    
    # Set parent scope variables
    set(DEB_SUITE "${DEB_SUITE_VAL}" PARENT_SCOPE)
    set(DEB_VERSION "${DEB_VERSION_VAL}" PARENT_SCOPE)
    set(DEB_RELEASE "${DEB_RELEASE_VAL}" PARENT_SCOPE)
    
    # Generate filename
    set(FILENAME "${DEB_CONFIG_PACKAGE_NAME}_${DEB_CONFIG_VERSION}_${DEB_RELEASE_VAL}_${DEB_CONFIG_ARCHITECTURE}.deb")
    set(CPACK_DEBIAN_FILE_NAME "${FILENAME}" PARENT_SCOPE)
    
    # Generate runtime and development package filenames if specified
    if(DEFINED DEB_CONFIG_RUNTIME_PACKAGE_NAME)
        set(RUNTIME_FILENAME "${DEB_CONFIG_RUNTIME_PACKAGE_NAME}_${DEB_CONFIG_VERSION}_${DEB_RELEASE_VAL}_${DEB_CONFIG_ARCHITECTURE}.deb")
        set(CPACK_DEBIAN_RUNTIME_FILE_NAME "${RUNTIME_FILENAME}" PARENT_SCOPE)
    endif()
    
    if(DEFINED DEB_CONFIG_DEVELOPMENT_PACKAGE_NAME)
        set(DEVELOPMENT_FILENAME "${DEB_CONFIG_DEVELOPMENT_PACKAGE_NAME}_${DEB_CONFIG_VERSION}_${DEB_RELEASE_VAL}_${DEB_CONFIG_ARCHITECTURE}.deb")
        set(CPACK_DEBIAN_DEVELOPMENT_FILE_NAME "${DEVELOPMENT_FILENAME}" PARENT_SCOPE)
    endif()
    
    # Debug output
    message(STATUS "DEB Package Filename Configuration:")
    message(STATUS "  Suite: ${DEB_SUITE_VAL}")
    message(STATUS "  Version: ${DEB_VERSION_VAL}")
    message(STATUS "  Release: ${DEB_RELEASE_VAL}")
    message(STATUS "  Architecture: ${DEB_CONFIG_ARCHITECTURE}")
    message(STATUS "  Filename: ${FILENAME}")
    if(DEFINED DEB_CONFIG_RUNTIME_PACKAGE_NAME)
        message(STATUS "  Runtime Filename: ${RUNTIME_FILENAME}")
    endif()
    if(DEFINED DEB_CONFIG_DEVELOPMENT_PACKAGE_NAME)
        message(STATUS "  Development Filename: ${DEVELOPMENT_FILENAME}")
    endif()
endfunction()
