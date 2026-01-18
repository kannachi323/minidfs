#include "auth_login_panel.h"
#include "imgui.h"
#include "util.h"
#include "asset_manager.h"
#include "app_view_registry.h"
#include <cstring>
#include <iostream>
#include "panel_ui.h"

namespace minidfs::panel {
    AuthLoginPanel::AuthLoginPanel(UIRegistry& registry)
        : registry_(registry) {
    }

    void AuthLoginPanel::render() {
        auto& state = registry_.get_state<AuthLoginState>("AuthLogin");

        ImGuiWindowFlags flags =
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoDocking;

        // Dark background
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(32.0f, 40.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 10.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12.0f, 8.0f));

        if (ImGui::Begin("AuthLogin", nullptr, flags)) {

            float window_width = ImGui::GetWindowWidth();
            float content_width = window_width - 64.0f; // Account for padding
            ImGui::SetNextItemWidth(content_width);

            show_header();
            show_form_fields(state);
            show_login_button(state);
            show_signup_link();
        }

        ImGui::End();
        ImGui::PopStyleVar(3);
        ImGui::PopStyleColor();
    }

    void AuthLoginPanel::show_header() {
        const char* text = "Welcome to Misty";
        
        // Scale font to 1.5x
        float original_scale = ImGui::GetFontSize();
        ImGui::SetWindowFontScale(1.5f);
        
        // Calculate text width and center it
        ImVec2 text_size = ImGui::CalcTextSize(text);
        float window_width = ImGui::GetWindowWidth();
        float center_x = (window_width - text_size.x) * 0.5f;
        ImGui::SetCursorPosX(center_x);
        
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
        ImGui::Text("%s", text);
        ImGui::PopStyleColor();
        
        // Restore original font scale
        ImGui::SetWindowFontScale(1.0f);
        
        ImGui::Spacing();
    }

    void AuthLoginPanel::show_form_fields(AuthLoginState& state) {
        float width = ImGui::GetContentRegionAvail().x;

        // Email
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 0.8f, 1.0f));
        auto& mail_icon = core::AssetManager::get().get_svg_texture("mail-16", 24);
        IconText(mail_icon, 16.0f, "Email", 2.0f, -2.0f);
        ImGui::PopStyleColor();
        ImGui::SetNextItemWidth(width);
        ImGui::InputTextWithHint("##email", "", state.email, sizeof(state.email));

        ImGui::Spacing();

        // Password
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 0.8f, 1.0f));
        auto& lock_icon = core::AssetManager::get().get_svg_texture("lock-16", 24);
        IconText(lock_icon, 16.0f, "Password", 1.0f, -2.0f);
        ImGui::PopStyleColor();
        ImGui::SetNextItemWidth(width);
        ImGui::InputTextWithHint("##password", "", state.password, sizeof(state.password), ImGuiInputTextFlags_Password);
    }

    void AuthLoginPanel::show_login_button(AuthLoginState& state) {
        float width = ImGui::GetContentRegionAvail().x;
        
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.9f, 1.0f)); // Blue button
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.6f, 1.0f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.15f, 0.4f, 0.8f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));

        if (ImGui::Button("Log in", ImVec2(width, 40))) {
            state.handle_login();
        }

        ImGui::PopStyleColor(4);
        ImGui::PopStyleVar();
    }

    void AuthLoginPanel::show_signup_link() {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
        ImGui::Text("Don't have an account? ");
        ImGui::SameLine();
        
        ImVec4 link_color = ImVec4(0.4f, 0.7f, 1.0f, 1.0f);
        ImGui::PushStyleColor(ImGuiCol_Text, link_color);
        ImGui::Text("Sign up");
        if (ImGui::IsItemHovered()) {
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            if (ImGui::IsItemClicked()) {
                core::AppViewRegistryController::switch_view(view::ViewID::Auth);
            }
        }
        ImGui::PopStyleColor(2);
    }
}
