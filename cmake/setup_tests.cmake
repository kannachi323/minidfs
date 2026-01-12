# cmake/setup_tests.cmake

file(GLOB_RECURSE TEST_SRCS
    "src/tests/*.cpp"
    "src/tests/*.h"
)
# tests
add_executable(minidfs_tests ${TEST_SRCS})
target_link_libraries(minidfs_tests PRIVATE
    minidfs
    gtest
    gtest_main
)