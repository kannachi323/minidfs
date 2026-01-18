#include "ts_view.h"
#include "imgui.h"

namespace minidfs::view {
    TSView::TSView(UIRegistry& ui_registry)
        : ui_registry_(ui_registry) {
        init_panels();
    }

    void TSView::init_panels() {
        ts_panel_ = std::make_shared<TSPanel>(ui_registry_);
    }

    ViewID TSView::get_view_id() {
        return ViewID::TS;
    }

    void TSView::render() {
        ImGuiViewport* viewport = ImGui::GetMainViewport();

        // Centered, compact window for tailscale view
        float window_width = 480.0f;
        float window_height = 520.0f;
        ImVec2 ts_size = ImVec2(window_width, window_height);
        ImVec2 ts_pos = ImVec2(
            viewport->WorkPos.x + (viewport->WorkSize.x - window_width) * 0.5f,
            viewport->WorkPos.y + (viewport->WorkSize.y - window_height) * 0.5f
        );

        ImGui::SetNextWindowPos(ts_pos, ImGuiCond_Always);
        ImGui::SetNextWindowSize(ts_size, ImGuiCond_Always);
        ImGui::SetNextWindowViewport(viewport->ID);
        ts_panel_->render();
    }
}
