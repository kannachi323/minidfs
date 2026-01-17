#include "navbar_panel.h"
#include "asset_manager.h"

namespace minidfs::panel {
    NavbarPanel::NavbarPanel(UIRegistry& ui_registry) : ui_registry_(ui_registry) {

    }

    void NavbarPanel::render() {
        auto& state = ui_registry_.get_state<NavbarState>("Navbar");

        ImGuiWindowFlags navbar_flags = ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoScrollbar;

        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.133f, 0.133f, 0.133f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

        if (ImGui::Begin("Navbar", nullptr, navbar_flags)) {
            show_logo_icon();

            ImGui::Dummy(ImVec2(0, 20));

            show_nav_item("home-24", "Home", 24, 0, state);
            show_nav_item("file-directory-24", "Folders", 24, 1, state);
            show_nav_item("bell-24", "Activity", 24, 2, state);

            float footer_y = ImGui::GetWindowHeight() - 80.0f;
            ImGui::SetCursorPosY(footer_y);
            show_nav_item("kebab-horizontal-24", "More", 24, 3, state);
        }
        ImGui::End();

        ImGui::PopStyleVar(); 
        ImGui::PopStyleColor(); 
    }

    void NavbarPanel::show_logo_icon() {
        const char* path = "assets/logo/mist_v1.png";
        const char* label = "mist_v1";
        
        auto& logo_image = AssetManager::get().get_image_texture(path);

        float logo_size = 48.0f;
        ImVec2 padding(8.0f, 8.0f);
        float button_size = logo_size + padding.x * 2.0f;

        float current_width = ImGui::GetWindowWidth();
        ImGui::SetCursorPosX((current_width - button_size) * 0.5f);

        ImGui::PushID("nav_logo");
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, padding);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 20.0f);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.18f, 0.18f, 0.18f, 1));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0));

        if (ImGui::ImageButton(label, (void*)(intptr_t)logo_image.id, ImVec2(logo_size, logo_size))) {
            auto& state = ui_registry_.get_state<NavbarState>("Navbar");
            state.handle_logo_click();
        }

        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar(2);
        ImGui::PopID();
    }

    void NavbarPanel::show_nav_item(const char* icon_name, const char* label, int size, int index, NavbarState& state) {
        bool is_selected = (state.selected_item == index);
        auto& icon = AssetManager::get().get_svg_texture(icon_name, size * 2);

        float navbar_width = ImGui::GetWindowWidth();

        float padding_x = 8.0f;
        float button_total_width = (float)size + (padding_x * 2.0f);
        float centered_x = (navbar_width - button_total_width) * 0.5f;

        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(padding_x, 8.0f));

        if (is_selected) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
        }
        else {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.11f, 0.11f, 0.11f, 0.0f));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
        }
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.18f, 0.18f, 0.18f, 1.0f));

        ImGui::SetCursorPosX(std::floor(centered_x));

        ImGuiID id = ImGui::GetID(label);
        if (ImGui::ImageButton(label, icon.id, ImVec2((float)size, (float)size))) {
            state.selected_item = index;
        }

        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar(2);

        float text_width = ImGui::CalcTextSize(label).x;
        float text_centered_x = (navbar_width - text_width) * 0.5f;

        ImGui::SetCursorPosX(std::floor(text_centered_x));

        ImVec4 textColor = is_selected ? ImVec4(1, 1, 1, 1) : ImVec4(0.6f, 0.6f, 0.6f, 1.0f);
        ImGui::PushStyleColor(ImGuiCol_Text, textColor);
        ImGui::Text("%s", label);
        ImGui::PopStyleColor();

        ImGui::Spacing();
    }
}