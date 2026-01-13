# --- 1. Protobuf/gRPC Setup (Keep this part) ---
find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)

set(PROTO_FILE ${CMAKE_SOURCE_DIR}/src/proto/minidfs.proto)
set(GEN_PATH ${CMAKE_SOURCE_DIR}/src/proto_src) # Better to put in Binary Dir than Source Dir
file(MAKE_DIRECTORY ${GEN_PATH})

get_target_property(PROTOC_BIN protobuf::protoc LOCATION)
get_target_property(GRPC_CPP_PLUGIN gRPC::grpc_cpp_plugin LOCATION)

set(GEN_FILES 
    "${GEN_PATH}/minidfs.pb.h" "${GEN_PATH}/minidfs.pb.cc"
    "${GEN_PATH}/minidfs.grpc.pb.h" "${GEN_PATH}/minidfs.grpc.pb.cc"
)

add_custom_command(
    OUTPUT ${GEN_FILES}
    COMMAND ${PROTOC_BIN} --cpp_out=${GEN_PATH} --grpc_out=${GEN_PATH}
            --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN}
            -I ${CMAKE_SOURCE_DIR}/src/proto ${PROTO_FILE}
    DEPENDS ${PROTO_FILE}
    COMMENT "Generating gRPC and Proto files"
)

# --- 2. Combined Library ---
file(GLOB_RECURSE MINIDFS_SRCS
    "src/dfs/*.cpp"
    "src/dfs/*.h"
    ${GEN_FILES}
)

add_library(minidfs STATIC ${MINIDFS_SRCS})

# --- 3. Clean Scoped Includes ---
target_include_directories(minidfs 
    PUBLIC 
        ${CMAKE_SOURCE_DIR}/src # Use one root
        $<BUILD_INTERFACE:${GEN_PATH}> # Allow finding generated headers
)

# --- 4. Linking ---
target_link_libraries(minidfs PUBLIC
    protobuf::libprotobuf
    gRPC::grpc++
)