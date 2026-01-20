# cmake/setup_gui.cmake
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/vendor/imgui)
set(IMGUI_SRCS
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)


file(GLOB_RECURSE APP_SRCS
    "src/application/*.cpp"
    "src/application/*.h"
    "vendor/glad/src/glad.cpp"
    ${IMGUI_SRCS}
)

add_executable(minidfs_client)
if(WIN32)
    file(GLOB WIN32_SRCS "src/application/platform/windows/*.cpp" "src/application/platform/windows/*.h")
    list(APPEND APP_SRCS ${WIN32_SRCS})
    target_sources(minidfs_client PRIVATE ${APP_SRCS})
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        set_target_properties(minidfs_client PROPERTIES WIN32_EXECUTABLE TRUE)
    else()
        set_target_properties(minidfs_client PROPERTIES WIN32_EXECUTABLE FALSE)
    endif()
    add_definitions(-DWIN32_LEAN_AND_MEAN)
    add_definitions(-DNOMINMAX)
    target_link_libraries(minidfs_client PRIVATE ws2_32 dwmapi)
    if(MSVC)
        # Use /MP for ALL configurations to speed up every build
        target_compile_options(minidfs_client PRIVATE /MP)

        # Edit and Continue flags (Use 'Debug' casing)
        target_compile_options(minidfs_client PRIVATE $<$<CONFIG:Debug>:/ZI>)
        target_link_options(minidfs_client PRIVATE $<$<CONFIG:Debug>:/EDITANDCONTINUE>)

        # CRITICAL: Fix the 'MDd' vs 'MT' mismatch error
        set_property(TARGET minidfs_client PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

        # Set Startup Project
        set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT minidfs_client)
        
        # Set Debugging Working Directory so it finds your config/assets
        set_target_properties(minidfs_client PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:minidfs_client>")
    endif()
elseif(APPLE)
    file(GLOB MAC_SRCS "application/platform/mac/*.cpp" "application/platform/mac/*.h")
    list(APPEND APP_SRCS ${MAC_SRCS})
    target_sources(minidfs_client PRIVATE ${APP_SRCS})
    target_link_libraries(minidfs_client PRIVATE
        "-framework CoreGraphics"
        "-framework CoreServices"
    )
elseif(UNIX AND NOT APPLE)
    file(GLOB LINUX_SRCS
        "src/application/platform/linux/*.cpp"
        "src/application/platform/linux/*.h"
    )
    list(APPEND APP_SRCS ${LINUX_SRCS})
    target_sources(minidfs_client PRIVATE ${APP_SRCS})
    target_compile_definitions(minidfs_client PRIVATE
        _GNU_SOURCE
    )
endif()

# vendor includes
target_include_directories(minidfs_client PRIVATE
    ${CMAKE_SOURCE_DIR}/vendor/glad/include
    ${CMAKE_SOURCE_DIR}/vendor/lunasvg/include
    ${CMAKE_SOURCE_DIR}/vendor/stb_image
    ${CMAKE_SOURCE_DIR}/vendor/imgui/backends
    ${CMAKE_SOURCE_DIR}/vendor/imgui
    ${CMAKE_SOURCE_DIR}/vendor/stb
    ${CMAKE_SOURCE_DIR}/vendor/json/single_include
)

# project includes
target_include_directories(minidfs_client PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${CMAKE_SOURCE_DIR}/src/proto_src
    ${CMAKE_SOURCE_DIR}/src/dfs
    ${CMAKE_SOURCE_DIR}/src/application
)

target_link_libraries(minidfs_client PRIVATE 
    minidfs
    glfw
    lunasvg
    OpenGL::GL
    CURL::libcurl
)

target_precompile_headers(minidfs_client PRIVATE 
    "src/application/minidfs.h"
)

add_custom_command(TARGET minidfs_client POST_BUILD
    # Operation 1: Copy main assets to build folder
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:minidfs_client>/assets"
    
    # Operation 2: Copy icons to the build folder's assets/icons directory
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/vendor/octicons/icons"
        "$<TARGET_FILE_DIR:minidfs_client>/assets/icons"

    COMMAND ${CMAKE_COMMAND} -E copy
        "${CMAKE_CURRENT_SOURCE_DIR}/minidfs.conf"
        "$<TARGET_FILE_DIR:minidfs_client>/minidfs.conf"
)




