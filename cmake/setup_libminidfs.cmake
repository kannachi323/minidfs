# cmake/setup_libminidfs.cmake

# miniDFS static library
set(MINIDFS_SRCS
    "dfs/file_manager.cpp" 
    "dfs/minidfs_impl.cpp"
    "dfs/minidfs_client.cpp"
    "dfs/pubsub_manager.cpp"
)

add_library(minidfs STATIC ${MINIDFS_SRCS})
target_link_libraries(minidfs PUBLIC minidfs_proto)
target_include_directories(minidfs PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${IMGUI_INCLUDE_DIRS}
    ${EXTERNAL_INCLUDE_DIRS}
)
