#include "ts_panel.h"
#include "imgui.h"
#include "core/util.h"
#include <cstring>

namespace minidfs::panel {
    TSPanel::TSPanel(UIRegistry& registry)
        : registry_(registry) {
    }

    void TSPanel::render() {
        auto& state = registry_.get_state<TSPanelState>("TSPanel");

        state.poll_status_if_needed();

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

        if (ImGui::Begin("TailscalePanel", nullptr, flags)) {

            float window_width = ImGui::GetWindowWidth();
            float content_width = window_width - 64.0f; // Account for padding
            ImGui::SetNextItemWidth(content_width);

            show_header();
            show_status_message(state);
            show_login_url(state);
            show_fetch_button(state);
            show_open_browser_button(state);
        }

        ImGui::End();
        ImGui::PopStyleVar(3);
        ImGui::PopStyleColor();
    }

    void TSPanel::show_header() {
        const char* text = "Tailscale Integration";
        
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
        ImGui::Spacing();
    }

    void TSPanel::show_status_message(TSPanelState& state) {
        if (!state.error_msg.empty()) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
            ImGui::TextWrapped("%s", state.error_msg.c_str());
            ImGui::PopStyleColor();
            ImGui::Spacing();
        }
        
        if (!state.success_msg.empty()) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 1.0f, 0.3f, 1.0f));
            ImGui::TextWrapped("%s", state.success_msg.c_str());
            ImGui::PopStyleColor();
            ImGui::Spacing();
        }
        
        if (state.is_fetching_url) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 0.8f, 1.0f));
            ImGui::Text("Fetching login URL...");
            ImGui::PopStyleColor();
            ImGui::Spacing();
        }

        if (state.has_fetched_url && !state.is_connected) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 0.8f, 1.0f));
            ImGui::TextWrapped("Finish signing in to Tailscale in your browser, then return here.");
            if (state.is_polling_status) {
                ImGui::Text("Checking connection status...");
            }
            ImGui::PopStyleColor();
            ImGui::Spacing();
        }
    }

    void TSPanel::show_login_url(TSPanelState& state) {
        if (!state.login_url.empty()) {
            float width = ImGui::GetContentRegionAvail().x;
            
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 0.8f, 1.0f));
            ImGui::Text("Login URL:");
            ImGui::PopStyleColor();
            ImGui::Spacing();
            
            // Display the URL in a read-only text field
            // Create a local buffer to avoid const_cast issues
            static char url_buffer[512] = "";
            strncpy(url_buffer, state.login_url.c_str(), sizeof(url_buffer) - 1);
            url_buffer[sizeof(url_buffer) - 1] = '\0';
            
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
            ImGui::InputText("##login_url", url_buffer, sizeof(url_buffer), ImGuiInputTextFlags_ReadOnly);
            ImGui::PopStyleColor();
            ImGui::Spacing();
        }
    }

    void TSPanel::show_fetch_button(TSPanelState& state) {
        float width = ImGui::GetContentRegionAvail().x;
        
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
        ImGui::PushStyleColor(ImGuiCol_Button, state.is_fetching_url ? ImVec4(0.3f, 0.3f, 0.3f, 1.0f) : ImVec4(0.2f, 0.5f, 0.9f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.6f, 1.0f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.15f, 0.4f, 0.8f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));

        const char* button_text = state.is_fetching_url ? "Fetching..." : "Get Tailscale Login URL";
        if (ImGui::Button(button_text, ImVec2(width, 40))) {
            if (!state.is_fetching_url) {
                state.handle_fetch_login_url();
            }
        }

        ImGui::PopStyleColor(4);
        ImGui::PopStyleVar();
        ImGui::Spacing();
    }

    void TSPanel::show_open_browser_button(TSPanelState& state) {
        if (!state.login_url.empty()) {
            float width = ImGui::GetContentRegionAvail().x;
            
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.4f, 1.0f)); // Green button
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.8f, 0.5f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.15f, 0.6f, 0.3f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));

            if (ImGui::Button("Open Login URL in Browser", ImVec2(width, 40))) {
                core::open_file_in_browser(state.login_url);
            }

            ImGui::PopStyleColor(4);
            ImGui::PopStyleVar();
        }
    }
}
