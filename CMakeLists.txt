cmake_minimum_required(VERSION 3.16)
cmake_policy(SET CMP0111 OLD)
project(minidfs LANGUAGES CXX C OBJCXX VERSION 1.0)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

include(setup_vendor)
include(setup_minidfs)
include(setup_server)
include(setup_tests)
include(setup_application)

