#include "auth_register_panel.h"
#include "imgui.h"
#include "util.h"
#include "asset_manager.h"
#include <cstring>
#include <iostream>

namespace minidfs::auth {
    AuthRegisterPanel::AuthRegisterPanel(UIRegistry& registry)
        : registry_(registry) {
    }

    void AuthRegisterPanel::render() {
        auto& state = registry_.get_state<AuthRegisterState>("AuthRegister");

        // Use monospace font for terminal aesthetic
        ImGuiIO& io = ImGui::GetIO();
        ImFont* mono_font = io.Fonts->Fonts[0]; // Default font (you can add monospace font to AssetManager if needed)

        ImGuiWindowFlags flags =
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoScrollbar;

        // Dark background
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(32.0f, 24.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 14.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12.0f, 8.0f));

        if (ImGui::Begin("AuthRegister", nullptr, flags)) {
            if (mono_font) ImGui::PushFont(mono_font);

            float window_width = ImGui::GetWindowWidth();
            float content_width = window_width - 64.0f; // Account for padding
            ImGui::SetNextItemWidth(content_width);

            show_header();
            show_form_fields(state);
            show_terms_checkbox(state);
            show_create_button(state);
            show_social_buttons();
            show_login_link();

            if (mono_font) ImGui::PopFont();

            // Status bar at bottom
            show_status_bar();
        }

        ImGui::End();
        ImGui::PopStyleVar(3);
        ImGui::PopStyleColor();
    }

    void AuthRegisterPanel::show_header() {
        // Green arrow and heading
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.8f, 0.4f, 1.0f)); // Green
        ImGui::Text("> ");
        ImGui::PopStyleColor();
        
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
        ImGui::Text("Create Account_");
        ImGui::PopStyleColor();

        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
        ImGui::Text("Initialize your secure workspace access.");
        ImGui::PopStyleColor();
        ImGui::Spacing();
        ImGui::Spacing();
    }

    void AuthRegisterPanel::show_form_fields(AuthRegisterState& state) {
        float width = ImGui::GetContentRegionAvail().x;

        // FULL NAME
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 0.8f, 1.0f));
        auto& person_icon = core::AssetManager::get().get_svg_texture("person-16", 16);
        ImGui::Image(person_icon.id, ImVec2(16, 16));
        ImGui::SameLine();
        ImGui::Text("FULL NAME:");
        ImGui::PopStyleColor();
        ImGui::SetNextItemWidth(width);
        ImGui::InputTextWithHint("##full_name", "John Doe", state.full_name, sizeof(state.full_name));

        ImGui::Spacing();

        // EMAIL ADDRESS
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 0.8f, 1.0f));
        auto& mail_icon = core::AssetManager::get().get_svg_texture("mail-16", 16);
        ImGui::Image(mail_icon.id, ImVec2(16, 16));
        ImGui::SameLine();
        ImGui::Text("EMAIL ADDRESS:");
        ImGui::PopStyleColor();
        ImGui::SetNextItemWidth(width);
        ImGui::InputTextWithHint("##email", "user@domain.com", state.email, sizeof(state.email));

        ImGui::Spacing();

        // PASSWORD
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 0.8f, 1.0f));
        auto& lock_icon = core::AssetManager::get().get_svg_texture("lock-16", 16);
        ImGui::Image(lock_icon.id, ImVec2(16, 16));
        ImGui::SameLine();
        ImGui::Text("PASSWORD:");
        ImGui::PopStyleColor();
        ImGui::SetNextItemWidth(width);
        ImGui::InputTextWithHint("##password", "........", state.password, sizeof(state.password), ImGuiInputTextFlags_Password);

        ImGui::Spacing();

        // CONFIRM PASSWORD
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 0.8f, 1.0f));
        auto& lock_icon2 = core::AssetManager::get().get_svg_texture("lock-16", 16);
        ImGui::Image(lock_icon2.id, ImVec2(16, 16));
        ImGui::SameLine();
        ImGui::Text("CONFIRM PASSWORD:");
        ImGui::PopStyleColor();
        ImGui::SetNextItemWidth(width);
        ImGui::InputTextWithHint("##confirm_password", "........", state.confirm_password, sizeof(state.confirm_password), ImGuiInputTextFlags_Password);

        ImGui::Spacing();
    }

    void AuthRegisterPanel::show_terms_checkbox(AuthRegisterState& state) {
        ImGui::Spacing();

        // Checkbox styling
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 0.8f, 1.0f));
        ImGui::Checkbox("I agree to the ", &state.agree_to_terms);
        ImGui::SameLine();
        
        // Underlined links - using Selectable to make them properly clickable
        ImVec4 link_color = ImVec4(0.4f, 0.7f, 1.0f, 1.0f);
        ImGui::PushStyleColor(ImGuiCol_Text, link_color);
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0, 0, 0, 0)); // Transparent background
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0, 0, 0, 0)); // Transparent hover
        
        if (ImGui::Selectable("Terms of Service", false, ImGuiSelectableFlags_None)) {
            this->open_terms_in_browser();
        }
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNone)) {
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        }
        
        ImGui::SameLine();
        ImGui::Text(" and ");
        ImGui::SameLine();
        
        if (ImGui::Selectable("Privacy Policy", false, ImGuiSelectableFlags_None)) {
            // Handle Privacy Policy click
        }
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNone)) {
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        }
        
        ImGui::PopStyleColor(4); // Pop: checkbox text, link text, header bg, header hover

        ImGui::Spacing();
    }

    void AuthRegisterPanel::show_create_button(AuthRegisterState& state) {
        float width = ImGui::GetContentRegionAvail().x;
        
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.9f, 1.0f)); // Blue button
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.6f, 1.0f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.15f, 0.4f, 0.8f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));

        if (ImGui::Button("Create account", ImVec2(width, 40))) {
            state.handle_create_account();
        }

        ImGui::PopStyleColor(4);
        ImGui::PopStyleVar();

        ImGui::Spacing();
        ImGui::Spacing();
    }

    void AuthRegisterPanel::show_social_buttons() {
        float width = ImGui::GetContentRegionAvail().x;
        float button_width = (width - ImGui::GetStyle().ItemSpacing.x) * 0.5f;

        // Divider with text
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
        ImGui::Separator();
        ImGui::Spacing();
        
        float text_width = ImGui::CalcTextSize("OR_SIGNUP_WITH").x;
        float start_x = (width - text_width) * 0.5f;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + start_x);
        ImGui::Text("OR_SIGNUP_WITH");
        ImGui::PopStyleColor();

        ImGui::Spacing();
        ImGui::Spacing();

        // GitHub button
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.25f, 0.25f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.35f, 0.35f, 0.35f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
        
        if (ImGui::Button("GitHub", ImVec2(button_width, 36))) {
            // Handle GitHub login
        }
        ImGui::PopStyleColor(3);

        ImGui::SameLine();

        // Google button
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.25f, 0.25f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.35f, 0.35f, 0.35f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
        
        if (ImGui::Button("Google", ImVec2(button_width, 36))) {
            // Handle Google login
        }
        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar();

        ImGui::Spacing();
        ImGui::Spacing();
    }

    void AuthRegisterPanel::show_login_link() {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
        ImGui::Text("Already have an account? ");
        ImGui::SameLine();
        
        ImVec4 link_color = ImVec4(0.4f, 0.7f, 1.0f, 1.0f);
        ImGui::PushStyleColor(ImGuiCol_Text, link_color);
        ImGui::Text("Log in");
        if (ImGui::IsItemHovered()) {
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            if (ImGui::IsItemClicked()) {
                // Handle login navigation
            }
        }
        ImGui::PopStyleColor(2);
    }

    void AuthRegisterPanel::open_terms_in_browser() {
        auto& state = registry_.get_state<AuthRegisterState>("AuthRegister");
        std::string html_path = state.terms_of_service_path;
        if (html_path.empty()) {
            std::cerr << "Warning: Could not find terms_of_service.html" << std::endl;
            return;
        }

        core::open_file_in_browser(html_path);
    }

    void AuthRegisterPanel::show_status_bar() {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        float status_height = 24.0f;
        ImVec2 status_pos = ImVec2(0, ImGui::GetWindowHeight() - status_height);
        ImVec2 status_size = ImVec2(ImGui::GetWindowWidth(), status_height);

        ImGui::SetCursorPos(status_pos);
        
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.08f, 0.08f, 0.08f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f, 4.0f));

        if (ImGui::BeginChild("##status_bar", status_size, false, ImGuiWindowFlags_NoScrollbar)) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
            
            // Left side: STATUS
            ImGui::Text("STATUS: ");
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.8f, 0.0f, 1.0f)); // Green
            ImGui::Text("READY");
            ImGui::PopStyleColor();

            // Right side: MEM
            float text_width = ImGui::CalcTextSize("MEM: 45MB").x;
            ImGui::SameLine(ImGui::GetWindowWidth() - text_width - 12.0f);
            ImGui::Text("MEM: 45MB");
            
            ImGui::PopStyleColor();
        }
        ImGui::EndChild();

        ImGui::PopStyleVar(3);
        ImGui::PopStyleColor();
    }
}