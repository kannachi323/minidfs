# cmake/setup_grpc.cmake

set(GRPC_DIR ${CMAKE_SOURCE_DIR}/proto_src)

set(GRPC_SRCS 
    ${GRPC_DIR}/minidfs.grpc.pb.cc
    ${GRPC_DIR}/minidfs.pb.cc
)

set(GRPC_INCLUDE_DIRS
    ${CMAKE_SOURCE_DIR}/proto_src
)