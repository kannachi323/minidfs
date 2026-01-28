#include "services_panel.h"
#include "imgui.h"
#include "core/imgui_utils.h"
#include <nlohmann/json.hpp>
#include <cstring>
#include <thread>

namespace minidfs::panel {
    ServicesPanel::ServicesPanel(UIRegistry& registry)
        : registry_(registry) {
    }

    void ServicesPanel::render() {
        auto& state = registry_.get_state<ServicesState>("Services");

        ImGuiWindowFlags flags =
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoScrollbar;

        core::WithWindowStyle(ImVec4(0.18f, 0.18f, 0.18f, 1.0f), ImVec2(32.0f, 24.0f), [&]() {
        if (ImGui::Begin("ServicesPanel", nullptr, flags)) {
            show_header();
            ImGui::Spacing();
            show_cloud_section(state);
            show_ms_login_modal(state);
            show_error_modal(state.error_msg, "ServicesError");
        }
        ImGui::End();
        });
    }

    void ServicesPanel::show_header() {
        ImGui::BeginGroup();
        core::WithFontScale(1.8f, []() {
            core::ColoredText(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Services");
        });
        core::ColoredText(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Manage your cloud storage services.");
        ImGui::EndGroup();
    }

    void ServicesPanel::show_cloud_section(ServicesState& state) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
        ImGui::Text("Cloud Storage");
        ImGui::PopStyleColor();

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 8.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 8.0f));

        // Show all OneDrive connections (including disconnected ones)
        std::vector<std::string> connection_ids;
        {
            std::lock_guard<std::mutex> lock(state.mu);
            for (const auto& conn : state.ms_connections) {
                if (!conn.profile.id.empty()) {
                    connection_ids.push_back(conn.profile.id);
                }
            }
        }

        // Display each connection card
        for (const auto& ms_user_id : connection_ids) {
            show_onedrive_profile_card(state, ms_user_id);
            ImGui::Spacing();
        }

        // Always show "Connect to OneDrive" button to allow adding more connections
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.35f, 0.5f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.45f, 0.6f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.15f, 0.3f, 0.45f, 1.0f));
        if (ImGui::Button("Add OneDrive", ImVec2(280.0f, 32))) {
            state.show_ms_login_modal = true;
        }
        ImGui::PopStyleColor(3);

        // Show error message if any
        {
            std::lock_guard<std::mutex> lock(state.mu);
            if (!state.error_msg.empty()) {
                ImGui::Spacing();
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
                ImGui::TextWrapped("%s", state.error_msg.c_str());
                ImGui::PopStyleColor();
            }
        }

        ImGui::PopStyleVar(2);
    }

    void ServicesPanel::show_onedrive_card_header(bool is_connected) {
        ImGui::BeginGroup();
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImVec2 cursor = ImGui::GetCursorScreenPos();
        if (is_connected) {
            draw_list->AddCircleFilled(ImVec2(cursor.x + 6.0f, cursor.y + 8.0f), 5.0f, IM_COL32(76, 175, 80, 255));
        } else {
            draw_list->AddCircleFilled(ImVec2(cursor.x + 6.0f, cursor.y + 8.0f), 5.0f, IM_COL32(255, 152, 0, 255));
        }
        ImGui::Dummy(ImVec2(16.0f, 0.0f));
        ImGui::SameLine();
        core::WithFontScale(1.2f, []() {
            core::ColoredText(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "OneDrive");
        });
        ImGui::SameLine();
        if (is_connected) {
            core::ColoredText(ImVec4(0.5f, 0.8f, 0.5f, 1.0f), "Connected");
        } else {
            core::ColoredText(ImVec4(1.0f, 0.6f, 0.2f, 1.0f), "Not Connected");
        }
        ImGui::EndGroup();
    }

    void ServicesPanel::show_onedrive_card_profile(const OneDriveCardState& card, const std::string& ms_user_id) {
        if (card.fetching || card.should_fetch) {
            core::ColoredText(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Loading account info...");
            return;
        }
        if (card.profile_loaded && (!card.profile.display_name.empty() || !card.profile.email.empty())) {
            if (!card.profile.display_name.empty()) {
                core::ColoredText(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Account");
                core::ColoredText(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%s", card.profile.display_name.c_str());
                ImGui::Spacing();
            }
            if (!card.profile.email.empty()) {
                core::ColoredText(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Email");
                core::ColoredText(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "%s", card.profile.email.c_str());
            }
            return;
        }
        core::ColoredText(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Account");
        core::ColoredText(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "ID: %s", ms_user_id.substr(0, 8).c_str());
    }

    void ServicesPanel::show_onedrive_card_actions(ServicesState& state, const std::string& ms_user_id, bool is_connected) {
        if (is_connected) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.2f, 0.2f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.6f, 0.3f, 0.3f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.45f, 0.15f, 0.15f, 1.0f));
            if (ImGui::Button("Disconnect", ImVec2(ImGui::GetContentRegionAvail().x, 28.0f))) {
                state.disconnect_onedrive(ms_user_id);
            }
            ImGui::PopStyleColor(3);
            return;
        }
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.35f, 0.5f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.45f, 0.6f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.15f, 0.3f, 0.45f, 1.0f));
        if (ImGui::Button("Reconnect", ImVec2(ImGui::GetContentRegionAvail().x * 0.48f, 28.0f))) {
            state.show_ms_login_modal = true;
        }
        ImGui::PopStyleColor(3);
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.25f, 0.25f, 0.25f, 1.0f));
        if (ImGui::Button("Remove", ImVec2(ImGui::GetContentRegionAvail().x, 28.0f))) {
            state.disconnect_onedrive(ms_user_id);
        }
        ImGui::PopStyleColor(3);
    }

    void ServicesPanel::show_onedrive_profile_card(ServicesState& state, const std::string& ms_user_id) {
        OneDriveCardState card;
        if (!state.get_onedrive_card_state(ms_user_id, card)) {
            return;
        }

        if (card.should_fetch && !card.current_token.empty()) {
            std::thread([&state, t = card.current_token]() {
                state.fetch_ms_user_profile(t);
            }).detach();
        }

        ImGui::PushID(ms_user_id.c_str());
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.0f);
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.14f, 0.14f, 0.14f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.3f, 0.3f, 0.3f, 0.5f));
        ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);

        if (ImGui::BeginChild("OneDriveCard", ImVec2(320.0f, 0.0f), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Borders)) {
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 6.0f));
            show_onedrive_card_header(card.is_connected);
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            show_onedrive_card_profile(card, ms_user_id);
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            show_onedrive_card_actions(state, ms_user_id, card.is_connected);
            ImGui::PopStyleVar();
        }
        ImGui::EndChild();

        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(2);
        ImGui::PopID();
    }

    void ServicesPanel::show_ms_login_modal(ServicesState& state) {
        if (state.show_ms_login_modal) {
            ImGui::OpenPopup("Connect to OneDrive");
        }

        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(480, 480), ImGuiCond_Appearing);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(24.0f, 24.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(12.0f, 12.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12.0f, 8.0f));

        if (ImGui::BeginPopupModal("Connect to OneDrive", &state.show_ms_login_modal,
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)) {

            ImGui::TextWrapped("To upload files to OneDrive, you need to authenticate with Microsoft.");
            ImGui::Spacing();
            ImGui::TextWrapped("1. Click 'Sign in to OneDrive' to open the browser and sign in to your Microsoft account");
            ImGui::TextWrapped("2. After signing in, return here.");
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            {
                std::lock_guard<std::mutex> lock(state.mu);
                if (!state.ms_auth_error.empty()) {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
                    ImGui::TextWrapped("%s", state.ms_auth_error.c_str());
                    ImGui::PopStyleColor();
                    ImGui::Spacing();
                }
                if (!state.success_msg.empty()) {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 0.8f, 0.4f, 1.0f));
                    ImGui::TextWrapped("%s", state.success_msg.c_str());
                    ImGui::PopStyleColor();
                    ImGui::Spacing();
                }
            }

            float button_width = ImGui::GetContentRegionAvail().x;
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.35f, 0.5f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.45f, 0.6f, 1.0f));
            if (ImGui::Button("Sign in to OneDrive", ImVec2(button_width, 36))) {
                state.initiate_ms_login();
            }
            ImGui::PopStyleColor(2);
            ImGui::Spacing();

            if (ImGui::Button("Done", ImVec2(button_width, 36))) {
                std::lock_guard<std::mutex> lock(state.mu);
                state.ms_auth_error.clear();
                state.success_msg.clear();
                state.show_ms_login_modal = false;
                state.check_connections();

                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
        ImGui::PopStyleVar(4);
    }

   
}
