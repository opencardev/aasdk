cmake_minimum_required(VERSION 3.5.1)

if( TARGET_ARCH STREQUAL "amd64" )
    message("Building for amd64")
elseif( TARGET_ARCH STREQUAL "armhf" )
    message("Building for armhf")
    set(CMAKE_C_COMPILER /usr/bin/arm-linux-gnueabihf-gcc-8)
    set(CMAKE_CXX_COMPILER /usr/bin/arm-linux-gnueabihf-g++-8)
	set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "armhf")
else()
    message("Unknown target architecture. Exiting")
    return()
endif()

set (aasdk_VERSION_MAJOR 3)
set (aasdk_VERSION_MINOR 1)
set (aasdk_VERSION_PATCH 0)

project(aasdk
VERSION ${aasdk_VERSION_MAJOR}.${aasdk_VERSION_MINOR}.${aasdk_VERSION_PATCH}
LANGUAGES CXX)

find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
endif()

set(base_directory ${CMAKE_CURRENT_SOURCE_DIR})
set(sources_directory ${base_directory}/src)

set(include_directory ${base_directory}/include)
set(include_ut_directory ${base_directory}/include_ut)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${base_directory}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${base_directory}/lib)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${base_directory}/bin)
set(EXECUTABLE_OUTPUT_PATH ${base_directory}/bin)

SET(CMAKE_CXX_STANDARD 14)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake_modules/")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS_INIT} -fPIC -Wall -pedantic")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "-g -O3")

set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)

add_definitions(-DBOOST_ALL_DYN_LINK)

include(${base_directory}/cmake_modules/gitversion.cmake)
set (aasdk_VERSION_PATCH ${_commit_timestamp})
set (CMAKE_PROJECT_VERSION_PATCH ${aasdk_VERSION_PATCH})

if(WIN32)
    set(WINSOCK2_LIBRARIES "ws2_32")
endif(WIN32)

if(AASDK_TEST)
    include(ExternalGtest)
endif(AASDK_TEST)

add_subdirectory(aasdk_proto)

find_package(Boost REQUIRED COMPONENTS system log OPTIONAL_COMPONENTS unit_test_framework)
find_package(libusb-1.0 REQUIRED)
find_package(Protobuf REQUIRED)
find_package(OpenSSL REQUIRED)

set(AASDK_PROTO_INCLUDE_DIRS ${CMAKE_CURRENT_BINARY_DIR})

include_directories(${AASDK_PROTO_INCLUDE_DIRS}
                        ${Boost_INCLUDE_DIRS}
                        ${PROTOBUF_INCLUDE_DIR}
                        ${OPENSSL_INCLUDE_DIR}
                        ${GTEST_INCLUDE_DIRS}
                        ${GMOCK_INCLUDE_DIRS}
                        ${include_directory}
                        ${include_ut_directory})

file(GLOB_RECURSE source_files ${sources_directory}/*.cpp)
file(GLOB_RECURSE include_files ${include_directory}/*.hpp)
file(GLOB_RECURSE tests_source_files ${sources_directory}/*.ut.cpp)
file(GLOB_RECURSE tests_include_files ${include_ut_directory}/*.hpp)

list(REMOVE_ITEM source_files ${tests_source_files})

add_library(aasdk SHARED
                ${source_files}
                ${include_files})

add_dependencies(aasdk aasdk_proto)
target_link_libraries(aasdk
                        aasdk_proto
                        libusb
                        ${Boost_LIBRARIES}
                        ${PROTOBUF_LIBRARIES}
                        ${OPENSSL_LIBRARIES}
                        ${WINSOCK2_LIBRARIES})

set(aasdk_VERSION_STRING ${aasdk_VERSION_MAJOR}.${aasdk_VERSION_MINOR}.${aasdk_VERSION_PATCH})
message(INFO " Project Version: ${aasdk_VERSION_STRING}")
set_target_properties(aasdk PROPERTIES VERSION ${aasdk_VERSION_STRING}
                                          SOVERSION ${aasdk_VERSION_MAJOR})

install(TARGETS aasdk DESTINATION lib COMPONENT libraries)
install(DIRECTORY include/aasdk DESTINATION include COMPONENT headers)

if(AASDK_TEST)
    add_executable(aasdk_ut
                    ${tests_source_files}
                    ${tests_include_files})

    add_dependencies(aasdk_ut aasdk)
    target_link_libraries(aasdk_ut
                            aasdk
                            ${GMOCK_LIBRARY_PATH}
                            ${GTEST_LIBRARY_PATH})

    if(AASDK_CODE_COVERAGE)
        include(CodeCoverage)
        append_coverage_compiler_flags()
        setup_target_for_coverage(NAME aasdk_coverage EXECUTABLE aasdk_ut DEPENDENCIES aasdk_ut)
    endif(AASDK_CODE_COVERAGE)
endif(AASDK_TEST)
SET(CPACK_GENERATOR "DEB")
SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "AASDK") #required
SET(CPACK_PACKAGE_VENDOR "AASDK") 
set(CPACK_PACKAGE_VERSION ${aasdk_VERSION_STRING})
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libusb-1.0-0,libboost-all-dev,libssl-dev,libprotobuf-dev")
set(CPACK_COMPONENTS_ALL libraries headers Unspecified)
set(CPACK_COMPONENT_LIBRARIES_DISPLAY_NAME "Libraries")
set(CPACK_COMPONENT_HEADERS_DISPLAY_NAME "C++ Headers")
set(CPACK_COMPONENT_LIBRARIES_DESCRIPTION
  "Static libraries used to build programs with AASDK")
set(CPACK_COMPONENT_HEADERS_DESCRIPTION
  "C/C++ header files for use with AASDK")
set(CPACK_COMPONENT_LIBRARIES_GROUP "Development")
set(CPACK_COMPONENT_HEADERS_GROUP "Development")
set(CPACK_COMPONENT_GROUP_DEVELOPMENT_EXPANDED ON)
set(CPACK_COMPONENT_GROUP_DEVELOPMENT_DESCRIPTION
  "All of the tools you'll ever need to develop software")
set(CPACK_COMPONENT_HEADERS_DEPENDS libraries)
set(CPACK_ALL_INSTALL_TYPES Full Developer)
set(CPACK_INSTALL_TYPE_FULL_DISPLAY_NAME "Everything")
set(CPACK_COMPONENT_LIBRARIES_INSTALL_TYPES Developer Full)
set(CPACK_COMPONENT_HEADERS_INSTALL_TYPES Developer Full)
INCLUDE(CPack)
