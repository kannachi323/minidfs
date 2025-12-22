# cmake/setup_imgui

set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/gui/imgui)

set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_metal.mm
)

file (GLOB OBJC_SRCS "")

set(IMGUI_INCLUDE_DIRS
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)