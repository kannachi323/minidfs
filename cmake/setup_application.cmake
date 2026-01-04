# cmake/setup_gui.cmake
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/gui/imgui)

set(IMGUI_SRCS
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
)

if(APPLE)
    list(APPEND IMGUI_SRCS ${IMGUI_DIR}/backends/imgui_impl_metal.mm)
else()
    list(APPEND IMGUI_SRCS ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp)
endif()

find_package(glfw3 REQUIRED)
add_library(imgui STATIC ${IMGUI_SRCS})
target_include_directories(imgui PUBLIC
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

if(APPLE)
    target_link_libraries(imgui PRIVATE
        glfw
        "-framework Cocoa"
        "-framework CoreFoundation"
        "-framework Metal"
        "-framework MetalKit"
        "-framework QuartzCore"
        "-framework IOKit"
        "-framework CoreGraphics"
    )
else()
    find_package(OpenGL REQUIRED)
    target_link_libraries(imgui PUBLIC
        glfw
        OpenGL::GL
    )
endif()

file(GLOB APP_SRCS
    "gui/application/*.cpp" 
    "gui/application/*.h" 
)
add_executable(minidfs_client)
if(WIN32 OR (UNIX AND NOT APPLE))
    list(APPEND APP_SRCS "gui/glad/src/glad.c")
    file(GLOB WIN32_SRCS "gui/application/windows/*.cpp" "gui/application/windows/*.h")
    list(APPEND APP_SRCS ${WIN32_SRCS})
    target_sources(minidfs_client PRIVATE ${APP_SRCS})
    set_target_properties(minidfs_client PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
elseif(APPLE)
    file(GLOB MAC_SRCS "gui/application/mac/*.mm" "gui/application/mac/*.h")
    list(APPEND APP_SRCS ${MAC_SRCS})
    target_sources(minidfs_client PRIVATE ${APP_SRCS})
    set_target_properties(minidfs_client PROPERTIES
        MACOSX_BUNDLE TRUE
    )
endif()
target_include_directories(minidfs_client PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/gui
    ${CMAKE_SOURCE_DIR}/gui/application
    ${CMAKE_SOURCE_DIR}/gui/application/mac
    ${CMAKE_SOURCE_DIR}/gui/application/windows
    ${CMAKE_SOURCE_DIR}/gui/glad/include
    ${IMGUI_INCLUDE_DIRS}
)
target_link_libraries(minidfs_client PRIVATE 
    minidfs
    imgui
    glfw
)


