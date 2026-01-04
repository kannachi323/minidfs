#include "mac_app.h"
#include "layout.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_metal.h"
#include <stdio.h>

#define GLFW_INCLUDE_NONE
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#import <Metal/Metal.h>
#import <QuartzCore/QuartzCore.h>

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

int MacApp::init()
{
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    float main_scale = ImGui_ImplGlfw_GetContentScaleForMonitor(glfwGetPrimaryMonitor());
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window_ = glfwCreateWindow((int)(1280 * main_scale), (int)(800 * main_scale), "MiniDFS Client", nullptr, nullptr);
    if (!window_) return -1;

    // --- Metal setup ---
    device_ = MTLCreateSystemDefaultDevice();
    command_queue_ = [device_ newCommandQueue];
    metal_layer_ = [CAMetalLayer layer];
    render_pass_desc_ = [MTLRenderPassDescriptor new];

    // Attach Metal layer to window
    NSWindow* nswin = glfwGetCocoaWindow(window_);
    metal_layer_.device = device_;
    metal_layer_.pixelFormat = MTLPixelFormatBGRA8Unorm;
    nswin.contentView.layer = metal_layer_;
    nswin.contentView.wantsLayer = YES;

    // --- ImGui setup ---
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(main_scale);
    style.FontScaleDpi = main_scale;
    io.ConfigDpiScaleFonts = true;
    io.ConfigDpiScaleViewports = true;
    ImGui::StyleColorsDark();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    ImGui_ImplGlfw_InitForOther(window_, true);
    ImGui_ImplMetal_Init(device_);

    return 0;
}

void MacApp::prepare_frame()
{
    glfwPollEvents();
    ImGui_ImplMetal_NewFrame(render_pass_desc_);
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void MacApp::render_frame()
{
    ImGui::Render();

    int w, h;
    glfwGetFramebufferSize(window_, &w, &h);
    metal_layer_.drawableSize = CGSizeMake(w, h);

    id<CAMetalDrawable> drawable = [metal_layer_ nextDrawable];
    id<MTLCommandBuffer> cmdBuf = [command_queue_ commandBuffer];

    render_pass_desc_.colorAttachments[0].texture = drawable.texture;
    render_pass_desc_.colorAttachments[0].loadAction = MTLLoadActionClear;
    render_pass_desc_.colorAttachments[0].storeAction = MTLStoreActionStore;
    render_pass_desc_.colorAttachments[0].clearColor = MTLClearColorMake(
        clear_color_.x * clear_color_.w,
        clear_color_.y * clear_color_.w,
        clear_color_.z * clear_color_.w,
        clear_color_.w
    );

    id<MTLRenderCommandEncoder> encoder = [cmdBuf renderCommandEncoderWithDescriptor:render_pass_desc_];
    ImGui_ImplMetal_RenderDrawData(ImGui::GetDrawData(), cmdBuf, encoder);
    [encoder endEncoding];

    [cmdBuf presentDrawable:drawable];
    [cmdBuf commit];
}

bool MacApp::is_running()
{
    return !glfwWindowShouldClose(window_);
}

void MacApp::cleanup()
{
    ImGui_ImplMetal_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    if (window_) glfwDestroyWindow(window_);
    glfwTerminate();

    device_ = nil;
    command_queue_ = nil;
    metal_layer_ = nil;
    render_pass_desc_ = nil;
}
