# cmake/vendor.cmake

find_package(gRPC CONFIG QUIET)
find_package(Protobuf CONFIG QUIET)
find_package(OpenGL REQUIRED)
find_package(CURL REQUIRED)

if(NOT gRPC_FOUND OR NOT Protobuf_FOUND OR  NOT OpenGL_FOUND)
    message(FATAL_ERROR 
        "\n--- DEPENDENCY MISSING ---\n"
        "gRPC, Protobuf, or OpenGL not found! Please install them using vcpkg:\n"
        "  vcpkg install grpc:x64-windows\n"
        "Then run CMake again with:\n"
        "  -DCMAKE_TOOLCHAIN_FILE=[path_to_vcpkg]/scripts/buildsystems/vcpkg.cmake\n"
        "--------------------------\n"
    )
endif()

# GLFW
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory(${CMAKE_SOURCE_DIR}/vendor/glfw EXCLUDE_FROM_ALL)

# LunaSVG
set(LUNASVG_BUILD_SHARED OFF CACHE BOOL "" FORCE)
add_subdirectory(${CMAKE_SOURCE_DIR}/vendor/lunasvg EXCLUDE_FROM_ALL)

# GoogleTest
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
add_subdirectory(${CMAKE_SOURCE_DIR}/vendor/googletest EXCLUDE_FROM_ALL)



message(STATUS "SDK fully loaded. No more compiler checks needed!")
