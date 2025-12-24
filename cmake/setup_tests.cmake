# cmake/setup_tests.cmake

enable_testing()

add_subdirectory(
    ${DEV_ROOT_PATH}/googletest
    ${CMAKE_BINARY_DIR}/gtest
    EXCLUDE_FROM_ALL
)

FILE(GLOB TEST_SRCS "tests/*.cpp")

add_executable(minidfs_tests
    ${TEST_SRCS}
)

target_link_libraries(minidfs_tests PRIVATE
    minidfs
    gtest_main
    gtest
    gRPC::grpc++
    protobuf::libprotobuf
)
