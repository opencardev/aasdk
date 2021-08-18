include(FindProtobuf)
find_package(Protobuf REQUIRED)
include_directories(${PROTOBUF_INCLUDE_DIR})

file(GLOB_RECURSE proto_files ${CMAKE_CURRENT_SOURCE_DIR}/*.proto)
protobuf_generate_cpp(proto_sources proto_headers ${proto_files})
add_library(aasdk_proto SHARED ${proto_headers} ${proto_sources})
target_link_libraries(aasdk_proto ${PROTOBUF_LIBRARIES})

set(aasdk_VERSION_STRING ${aasdk_VERSION_MAJOR}.${aasdk_VERSION_MINOR}.${aasdk_VERSION_PATCH})
set_target_properties(aasdk_proto PROPERTIES VERSION ${aasdk_VERSION_STRING}
                                          SOVERSION ${aasdk_VERSION_MAJOR})

install(TARGETS aasdk_proto DESTINATION lib)
install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} DESTINATION include
    FILES_MATCHING PATTERN *.h
    PATTERN CMakeFiles EXCLUDE )
