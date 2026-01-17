#ifdef __APPLE__
#define GLFW_INCLUDE_NONE
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "mac_app.h"
#include "asset_manager.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <cstdio>

namespace minidfs {
    void MacApp::init_platform() {
        init_glfw();
        init_window();
        init_opengl();
		setup_window_icon();
		init_imgui();
    }

    void MacApp::cleanup() {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        glfwDestroyWindow(window_);
        glfwTerminate();
    }
    void MacApp::init_glfw() {
        glfwSetErrorCallback(glfw_error_callback);
        if (!glfwInit()) throw std::runtime_error("Failed to initialize GLFW");
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    }

    void MacApp::init_window() {
        window_ = glfwCreateWindow(1280, 720, "MiniDFS Client", NULL, NULL);
        if (!window_) throw std::runtime_error("Failed to create GLFW window");
        glfwMakeContextCurrent(window_); //VERY IMPORTANT
        glfwSetWindowUserPointer(window_, this);
        glfwSetWindowSizeCallback(window_, glfw_window_size_callback);
    }

    void MacApp::init_opengl() {
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
            throw std::runtime_error("Failed to initialize GLAD");
        }
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    void MacApp::setup_window_icon() {
        GLFWimage images[1]; 
        
        int channels;
        images[0].pixels = stbi_load("assets/logo/mist_v1", &images[0].width, &images[0].height, &channels, 4); 

        if (images[0].pixels) {
            glfwSetWindowIcon(window_, 1, images);
            stbi_image_free(images[0].pixels); // Free memory after passing to GLFW
        }
    }

    void MacApp::init_imgui() {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        configure_imgui_io();
        configure_imgui_style();

        core::AssetManager::get().load_themes();
        core::AssetManager::get().load_fonts();

      
        ImGui_ImplGlfw_InitForOpenGL(window_, true);
        ImGui_ImplOpenGL3_Init(glsl_version_);
    }


    void MacApp::configure_imgui_io() {
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows

        // Experimental DPI features (GLFW 3.3+)
        if (GLFW_VERSION_MAJOR >= 3 && GLFW_VERSION_MINOR >= 3) {
            io.ConfigDpiScaleFonts = true;
            io.ConfigDpiScaleViewports = true;
        }
     
    }

    void MacApp::configure_imgui_style() {
        ImGui::StyleColorsDark();
        ImGuiStyle& style = ImGui::GetStyle();
        style.FrameRounding = 6.0f;
        style.GrabRounding = 6.0f;
        style.ScrollbarRounding = 6.0f;
	}

    void MacApp::prepare_frame() {
        glfwPollEvents();
        int display_w, display_h;
        glfwGetFramebufferSize(window_, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }

    void MacApp::render_frame() {
        ImGui::Render();
        
        int w, h;
        glfwGetFramebufferSize(window_, &w, &h);
        if (w == 0 || h == 0)
            return;

        glfwMakeContextCurrent(window_);
        glViewport(0, 0, w, h);
        glClearColor(0.11f, 0.11f, 0.11f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        
		ImGuiIO& io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(window_);
        }

        glfwSwapBuffers(window_);
    }

    bool MacApp::is_running() {
        return !glfwWindowShouldClose(window_);
    }

    void MacApp::glfw_error_callback(int error, const char* description) {
        fprintf(stderr, "GLFW Error %d: %s\n", error, description);
    }

    void MacApp::glfw_window_size_callback(GLFWwindow* window, int width, int height) {

    }
}
#endif