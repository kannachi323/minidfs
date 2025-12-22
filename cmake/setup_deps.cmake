# cmake/setup_deps.cmake

if(NOT DEFINED ENV{DEV_ROOT})
    message(FATAL_ERROR "DEV_ROOT_PATH not found! Set DEV_ROOT env var.")
endif()

set(DEV_ROOT_PATH $ENV{DEV_ROOT})
message(STATUS "DEV_ROOT_PATH = ${DEV_ROOT_PATH}")


message(STATUS "Dependencies found at: ${DEV_ROOT_PATH}")

set(EXTERNAL_INCLUDE_DIRS
    /opt/homebrew/include
    ${DEV_ROOT_PATH}/glfw/include
)

set(EXTERNAL_LIBS
    ${CMAKE_SOURCE_DIR}/libs/libglfw3.a
)