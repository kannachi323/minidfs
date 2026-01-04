#pragma once
#include <GLFW/glfw3.h>
#include "imgui.h"

class Application {
public:
    virtual ~Application() = default;

    // Pure virtual functions (the "Interface")
    virtual int init() = 0;
    virtual void prepare_frame() = 0;
    virtual void render_frame() = 0;
    virtual bool is_running() = 0;
    virtual void cleanup() = 0;

protected:
    // Move variables here so WindowsApp can inherit them
    GLFWwindow* window_ = nullptr;
    const char* glsl_version_ = "#version 130";
    ImVec4 clear_color_ = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
};