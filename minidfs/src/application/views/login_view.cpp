#include "login_view.h"
#include "imgui.h"

namespace minidfs::view {
    LoginView::LoginView(UIRegistry& ui_registry)
        : ui_registry_(ui_registry) {
        init_panels();
    }

    void LoginView::init_panels() {
        auth_login_panel_ = std::make_shared<AuthLoginPanel>(ui_registry_);
    }

    ViewID LoginView::get_view_id() {
        return ViewID::Login;
    }

    void LoginView::render() {
        ImGuiViewport* viewport = ImGui::GetMainViewport();

        // Centered, compact window for login view
        float window_width = 480.0f;
        float window_height = 520.0f;
        ImVec2 login_size = ImVec2(window_width, window_height);
        ImVec2 login_pos = ImVec2(
            viewport->WorkPos.x + (viewport->WorkSize.x - window_width) * 0.5f,
            viewport->WorkPos.y + (viewport->WorkSize.y - window_height) * 0.5f
        );

        ImGui::SetNextWindowPos(login_pos, ImGuiCond_Always);
        ImGui::SetNextWindowSize(login_size, ImGuiCond_Always);
        ImGui::SetNextWindowViewport(viewport->ID);
        auth_login_panel_->render();
    }
}
