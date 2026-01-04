#pragma once
#include "application.h"
#include "imgui.h"
#include <GLFW/glfw3.h>


class MacApp : public Application {
public:
    int init() override;
    void prepare_frame() override;
    void render_frame() override;
    bool is_running() override;
    void cleanup() override;

private:
    GLFWwindow* window_ = nullptr;

    ImVec4 clear_color_ = ImVec4(0.45f, 0.55f, 0.60f, 1.0f);

#ifdef __APPLE__
    id<MTLDevice> device_ = nil;
    id<MTLCommandQueue> command_queue_ = nil;
    CAMetalLayer* metal_layer_ = nil;
    MTLRenderPassDescriptor* render_pass_desc_ = nil;
#endif
};
