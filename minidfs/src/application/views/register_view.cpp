#include "register_view.h"
#include "imgui.h"

namespace minidfs::view {
    RegisterView::RegisterView(UIRegistry& ui_registry)
        : ui_registry_(ui_registry) {
        init_panels();
    }

    void RegisterView::init_panels() {
        auth_register_panel_ = std::make_shared<AuthRegisterPanel>(ui_registry_);
    }

    ViewID RegisterView::get_view_id() {
        return ViewID::Auth;
    }

    void RegisterView::render() {
        ImGuiViewport* viewport = ImGui::GetMainViewport();

        // Centered, compact window for register view
        float window_width = 480.0f;
        float window_height = 620.0f;
        ImVec2 register_size = ImVec2(window_width, window_height);
        ImVec2 register_pos = ImVec2(
            viewport->WorkPos.x + (viewport->WorkSize.x - window_width) * 0.5f,
            viewport->WorkPos.y + (viewport->WorkSize.y - window_height) * 0.5f
        );

        ImGui::SetNextWindowPos(register_pos, ImGuiCond_Always);
        ImGui::SetNextWindowSize(register_size, ImGuiCond_Always);
        ImGui::SetNextWindowViewport(viewport->ID);
        auth_register_panel_->render();
    }
}
