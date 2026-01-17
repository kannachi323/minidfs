#include "auth_view.h"
#include "imgui.h"

namespace minidfs::view {
    AuthView::AuthView(UIRegistry& ui_registry)
        : ui_registry_(ui_registry) {
        init_panels();
    }

    void AuthView::init_panels() {
        auth_register_panel_ = std::make_shared<AuthRegisterPanel>(ui_registry_);
    }

    ViewID AuthView::get_view_id() {
        return ViewID::Auth;
    }

    void AuthView::render() {
        ImGuiViewport* viewport = ImGui::GetMainViewport();

        // Centered, compact window for auth view
        float window_width = 520.0f;
        float window_height = 680.0f;
        ImVec2 auth_size = ImVec2(window_width, window_height);
        ImVec2 auth_pos = ImVec2(
            viewport->WorkPos.x + (viewport->WorkSize.x - window_width) * 0.5f,
            viewport->WorkPos.y + (viewport->WorkSize.y - window_height) * 0.5f
        );

        ImGui::SetNextWindowPos(auth_pos, ImGuiCond_Always);
        ImGui::SetNextWindowSize(auth_size, ImGuiCond_Always);
        auth_register_panel_->render();
    }
}