cmake_minimum_required(VERSION 2.8.12)

# Ensure that the proto libraries are installed.
find_package(Protobuf REQUIRED)

if(NOT PROTOBUF_FOUND)
  message(FATAL_ERROR "Please install the protobuf libraries.")
endif()

include_directories(${CMAKE_CURRENT_BINARY_DIR})

# Generate code for the abstract syntax tree proto.
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ast.proto)
add_library(ast_proto STATIC ${PROTO_SRCS} ${PROTO_HDRS})

