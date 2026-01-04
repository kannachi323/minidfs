cmake_minimum_required(VERSION 3.10)
project(minidfs LANGUAGES CXX C VERSION 1.0)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

include(setup_proto)
include(setup_libminidfs)
include(setup_server)
include(setup_tests)
include(setup_application)
