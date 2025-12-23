# cmake/setup_tests.cmake

enable_testing()

add_subdirectory(
    ${DEV_ROOT_PATH}/googletest
    ${CMAKE_BINARY_DIR}/gtest
    EXCLUDE_FROM_ALL
)

FILE(GLOB TEST_SRCS "tests/*.cpp")

add_executable(dfs_tests
    ${TEST_SRCS}
)

target_link_libraries(dfs_tests PRIVATE
    minidfs
    gtest_main
    gtest
    gRPC::grpc++
    protobuf::libprotobuf
)

add_test(
    NAME dfs_file_manager_tests
    COMMAND dfs_tests
)

set_tests_properties(dfs_file_manager_tests PROPERTIES
    TIMEOUT 10
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)