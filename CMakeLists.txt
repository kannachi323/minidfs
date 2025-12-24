cmake_minimum_required(VERSION 3.10)
project(minidfs LANGUAGES CXX C OBJCXX VERSION 1.0)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build)
set(CMAKE_CXX_STANDARD 20)

set(CMAKE_PREFIX_PATH /opt/homebrew)
find_package(absl REQUIRED HINTS /opt/homebrew)
find_package(gRPC REQUIRED HINTS /opt/homebrew)
find_package(Protobuf REQUIRED HINTS /opt/homebrew)
find_package(openssl REQUIRED HINTS /opt/homebrew)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

include(setup_proto)
set(COMMON_SRCS 
    "dfs/file_manager.cpp" 
    "dfs/minidfs_impl.cpp"
    "dfs/minidfs_client.cpp"
    "dfs/pubsub_manager.cpp"
    ${GRPC_SRCS}
)

# miniDFS static library
add_library(minidfs STATIC ${COMMON_SRCS})
target_link_libraries(minidfs PUBLIC OpenSSL::Crypto)
include(setup_imgui)
include(setup_deps)
target_include_directories(minidfs PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${IMGUI_INCLUDE_DIRS}
    ${GRPC_INCLUDE_DIRS}
    ${EXTERNAL_INCLUDE_DIRS}
)

# SERVER
add_executable(minidfs_server "dfs/server.cpp" ${IMGUI_SOURCES})
target_link_libraries(minidfs_server PRIVATE 
    minidfs
    ${EXTERNAL_LIBS}
    gRPC::grpc++
    protobuf::libprotobuf
    absl::status
    absl::strings
    absl::synchronization
)

# CLIENT
add_executable(minidfs_client "dfs/client.cpp")
target_link_libraries(minidfs_client PRIVATE 
    minidfs
    gRPC::grpc++
    protobuf::libprotobuf
    absl::status
    absl::strings
)

# TESTING
include(setup_tests)

if(APPLE)
    foreach(target minidfs_server minidfs_client)
        target_link_libraries(${target} PRIVATE
            "-framework Cocoa"
            "-framework CoreFoundation"
            "-framework Metal"
            "-framework MetalKit"
            "-framework QuartzCore"
            "-framework IOKit"
            "-framework CoreGraphics"
        )
    endforeach()
endif()