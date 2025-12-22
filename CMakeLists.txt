cmake_minimum_required(VERSION 3.10)
project(minidfs LANGUAGES CXX C OBJCXX VERSION 1.0)

set(TARGET minidfs)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build)
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_PREFIX_PATH /opt/homebrew)
find_package(absl REQUIRED HINTS /opt/homebrew)
find_package(gRPC REQUIRED HINTS /opt/homebrew)
find_package(Protobuf REQUIRED HINTS /opt/homebrew)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
include(setup_imgui)
include(setup_deps)
include(setup_grpc)

file(GLOB CPP_SRCS "dfs/*.cpp")
file(GLOB C_SRCS "dfs/*.c")

add_executable(${TARGET} ${CPP_SRCS} ${C_SRCS} ${IMGUI_SOURCES} ${GRPC_SRCS})

target_include_directories(${TARGET} PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${IMGUI_INCLUDE_DIRS}
    ${GRPC_INCLUDE_DIRS}
    ${EXTERNAL_INCLUDE_DIRS}
)

if(APPLE)
    target_link_libraries(${TARGET} PRIVATE
        "-framework Cocoa"
        "-framework CoreFoundation"
        "-framework Metal"
        "-framework MetalKit"
        "-framework QuartzCore"
        "-framework IOKit"
        "-framework CoreGraphics"
    )
endif()


target_link_libraries(${TARGET} PRIVATE
    ${EXTERNAL_LIBS}
    absl::log_internal_message
    absl::log_internal_check_op
    absl::log_internal_nullguard
    absl::status
    absl::synchronization
    absl::strings
    gRPC::grpc++
    gRPC::grpc
    gRPC::gpr
    protobuf::libprotobuf
)
