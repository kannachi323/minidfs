#include "devices_panel.h"
#include "imgui.h"
#include "panels/setup/ts_panel_state.h"
#include "panels/file_explorer/workspace_state.h"
#include "core/imgui_utils.h"
#include <sstream>
#include <iomanip>
#include <cstring>
#include <algorithm>

namespace minidfs::panel {
    DevicesPanel::DevicesPanel(UIRegistry& registry)
        : registry_(registry) {
        ts_panel_ = std::make_shared<TSPanel>(registry_);
    }

    void DevicesPanel::render() {
        auto& state = registry_.get_state<DevicesState>("Devices");
        auto& workspace_state = registry_.get_state<WorkspaceState>("Workspace");

        // Update workspace filter before fetching
        state.current_workspace_id = workspace_state.get_current_workspace_id();

        // Fetch devices periodically
        state.fetch_devices();

        ImGuiWindowFlags flags =
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoScrollbar;

        core::WithWindowStyle(ImVec4(0.18f, 0.18f, 0.18f, 1.0f), ImVec2(32.0f, 24.0f), [&]() {
        if (ImGui::Begin("DevicesPanel", nullptr, flags)) {
            show_header();
            ImGui::Spacing();
            show_filters(state);
            ImGui::Spacing();
            show_devices_list(state);
            show_add_device_modal(state);
            show_edit_device_modal(state);
            show_delete_confirm_modal(state);
            show_error_modal(state.error_msg, "DevicesError");
        }

        ImGui::End();
        });
    }

    void DevicesPanel::show_header() {
        ImGui::BeginGroup();
        core::WithFontScale(1.8f, []() {
            core::ColoredText(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Devices");
        });
        
        core::ColoredText(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "View and manage all connected devices.");
        ImGui::EndGroup();
        
        // Add Device button on the right
        ImGui::SameLine(0, 20);
        float button_width = 180.0f;
        float window_width = ImGui::GetWindowWidth();
        float button_x = window_width - button_width - 32.0f;
        ImGui::SetCursorPosX(button_x);
        ImGui::SetCursorPosY(0);
        
        auto& state = registry_.get_state<DevicesState>("Devices");
        if (core::StyledButton("+ Add Device", ImVec2(button_width, 36), core::ButtonTheme::Primary())) {
            state.show_add_device_modal = true;
            // Reset TSPanelState when opening the modal and disable view switching
            auto& ts_state = registry_.get_state<TSPanelState>("TSPanel");
            ts_state.clear_state();
            ts_state.should_switch_view_on_register = false; // Don't switch views when used in modal
        }
    }

    void DevicesPanel::show_filters(DevicesState& state) {
        ImGui::BeginGroup();

        // Search box
        ImGui::SetNextItemWidth(300.0f);
        ImGui::InputTextWithHint("##search", "Search devices...", state.search_buffer, IM_ARRAYSIZE(state.search_buffer));

        ImGui::SameLine();

        // Filter online only checkbox
        ImGui::Checkbox("Online Only", &state.filter_online_only);

        ImGui::EndGroup();
    }

    void DevicesPanel::show_devices_list(DevicesState& state) {
        core::CustomStyleVar item_spacing(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 8.0f));

        // Filter devices based on search and filter
        std::vector<DeviceInfo*> filtered_devices;
        for (auto& device : state.devices) {
            // Search filter
            bool matches_search = strlen(state.search_buffer) == 0;
            if (!matches_search) {
                std::string search_lower = state.search_buffer;
                std::transform(search_lower.begin(), search_lower.end(), search_lower.begin(), ::tolower);
                std::string name_lower = device.name;
                std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(), ::tolower);
                matches_search = name_lower.find(search_lower) != std::string::npos;
            }

            // Online filter
            bool matches_online = !state.filter_online_only || device.is_online;

            if (matches_search && matches_online) {
                filtered_devices.push_back(&device);
            }
        }

        if (filtered_devices.empty()) {
            core::ColoredText(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No devices found.");
        } else {
            for (auto* device : filtered_devices) {
                show_device_item(*device);
            }
        }
    }

    void DevicesPanel::show_device_item(DeviceInfo& device) {
        auto& state = registry_.get_state<DevicesState>("Devices");

        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.22f, 0.22f, 0.22f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16.0f, 12.0f));

        // Use NoScrollbar to prevent scrollbar from appearing
        ImGui::BeginChild(device.id.c_str(), ImVec2(-1, 100), true, ImGuiWindowFlags_NoScrollbar);
        {
            float content_width = ImGui::GetContentRegionAvail().x;
            float window_padding_x = 16.0f;

            // Check if mouse is hovering over this card
            bool is_hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);

            // === Header Row: Status dot + Name + Status badge (always visible) ===
            ImGui::BeginGroup();

            // Status indicator dot
            ImVec4 status_color = device.is_online ? ImVec4(0.3f, 0.8f, 0.4f, 1.0f) : ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
            core::ColoredText(status_color, "●");
            ImGui::SameLine(0, 8);

            // Device name
            core::WithFontScale(1.15f, [&]() {
                core::ColoredText(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%s", device.name.c_str());
            });

            ImGui::EndGroup();

            // Status badge (always visible, top-right)
            float badge_width = 70.0f;
            float badge_x = content_width - badge_width;

            ImGui::SameLine();
            ImGui::SetCursorPosX(badge_x);

            ImVec4 badge_bg = device.is_online ? ImVec4(0.15f, 0.35f, 0.2f, 1.0f) : ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
            ImVec4 badge_text = device.is_online ? ImVec4(0.4f, 0.9f, 0.5f, 1.0f) : ImVec4(0.6f, 0.6f, 0.6f, 1.0f);
            ImGui::PushStyleColor(ImGuiCol_Button, badge_bg);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, badge_bg);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, badge_bg);
            ImGui::PushStyleColor(ImGuiCol_Text, badge_text);
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
            ImGui::Button(device.is_online ? "Online" : "Offline", ImVec2(badge_width, 22));
            ImGui::PopStyleVar();
            ImGui::PopStyleColor(4);

            // Store cursor position after header for info grid
            float info_start_y = ImGui::GetCursorPosY();

            // === Edit/Delete buttons below status badge (only on hover, absolute positioned) ===
            if (is_hovered) {
                float edit_btn_width = 50.0f;
                float delete_btn_width = 60.0f;
                float btn_spacing = 6.0f;
                float buttons_total_width = edit_btn_width + delete_btn_width + btn_spacing;
                float buttons_x = content_width - buttons_total_width;
                float buttons_y = info_start_y + 2.0f; // Position below status badge

                // Save current cursor, position buttons absolutely, then restore
                ImVec2 saved_cursor = ImGui::GetCursorPos();
                ImGui::SetCursorPos(ImVec2(buttons_x, buttons_y));

                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);

                // Edit button
                std::string edit_id = "edit_" + device.id;
                ImGui::PushID(edit_id.c_str());
                if (core::StyledButton("Edit", ImVec2(edit_btn_width, 22), core::ButtonTheme::Primary())) {
                    state.show_edit_device_modal = true;
                    state.editing_device_id = device.id;
                    strncpy(state.editing_device_name, device.name.c_str(), sizeof(state.editing_device_name) - 1);
                    state.editing_device_name[sizeof(state.editing_device_name) - 1] = '\0';
                    strncpy(state.editing_mount_path, device.mount_path.c_str(), sizeof(state.editing_mount_path) - 1);
                    state.editing_mount_path[sizeof(state.editing_mount_path) - 1] = '\0';
                }
                ImGui::PopID();

                ImGui::SameLine(0, btn_spacing);

                // Delete button
                std::string delete_id = "delete_" + device.id;
                ImGui::PushID(delete_id.c_str());
                if (core::StyledButton("Delete", ImVec2(delete_btn_width, 22), core::ButtonTheme::Danger())) {
                    state.show_delete_confirm = true;
                    state.deleting_device_id = device.id;
                    state.deleting_device_name = device.name;
                }
                ImGui::PopID();

                ImGui::PopStyleVar();

                // Restore cursor position for info grid
                ImGui::SetCursorPos(saved_cursor);
            }

            // === Info Grid (2 columns) ===
            ImGui::Spacing();
            float col_width = content_width * 0.5f;

            core::WithTextColor(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), [&]() {
                // Row 1: ID and IP
                ImGui::Text("ID:");
                ImGui::SameLine(0, 4);
                core::ColoredText(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "%s", device.id.c_str());

                if (!device.ip_address.empty()) {
                    ImGui::SameLine();
                    ImGui::SetCursorPosX(col_width);
                    ImGui::Text("IP:");
                    ImGui::SameLine(0, 4);
                    core::ColoredText(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "%s", device.ip_address.c_str());
                }

                // Row 2: Mount path and Last seen
                if (!device.mount_path.empty() || !device.last_seen.empty()) {
                    if (!device.mount_path.empty()) {
                        ImGui::Text("Mount:");
                        ImGui::SameLine(0, 4);
                        core::ColoredText(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "%s", device.mount_path.c_str());
                    }

                    if (!device.last_seen.empty()) {
                        if (!device.mount_path.empty()) {
                            ImGui::SameLine();
                            ImGui::SetCursorPosX(col_width);
                        }
                        ImGui::Text("Last seen:");
                        ImGui::SameLine(0, 4);
                        core::ColoredText(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "%s", device.last_seen.c_str());
                    }
                }

                // Row 3: Storage with progress bar (if available)
                if (device.total_bytes > 0) {
                    double used_gb = (double)device.used_bytes / (1024.0 * 1024.0 * 1024.0);
                    double total_gb = (double)device.total_bytes / (1024.0 * 1024.0 * 1024.0);
                    float usage_fraction = (float)device.used_bytes / (float)device.total_bytes;

                    std::stringstream ss;
                    ss << std::fixed << std::setprecision(1) << used_gb << " GB / " << total_gb << " GB";

                    ImGui::Text("Storage:");
                    ImGui::SameLine(0, 4);
                    core::ColoredText(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "%s", ss.str().c_str());

                    ImGui::SameLine(0, 12);
                    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.3f, 0.6f, 0.9f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
                    ImGui::ProgressBar(usage_fraction, ImVec2(120, 14), "");
                    ImGui::PopStyleColor(2);
                }
            });
        }
        ImGui::EndChild();

        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor();
    }

    void DevicesPanel::show_add_device_modal(DevicesState& state) {
        if (state.show_add_device_modal) {
            ImGui::OpenPopup("Add Device");
        }

        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(500, 500), ImGuiCond_Appearing);

        core::CustomStyleVar padding(ImGuiStyleVar_WindowPadding, ImVec2(24.0f, 24.0f));
        core::CustomStyleVar spacing(ImGuiStyleVar_ItemSpacing, ImVec2(12.0f, 16.0f));

        if (ImGui::BeginPopupModal("Add Device", &state.show_add_device_modal, 
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize)) {
            
            // Header
            core::WithFontScale(1.3f, []() {
                core::ColoredText(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Add New Device");
            });
            
            ImGui::Spacing();
            core::ColoredText(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Connect a new device using Tailscale. Follow the steps below to authenticate and add your device.");
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            
            // Use TSPanel to handle the Tailscale setup flow
            // Render just the content without creating a window
            ts_panel_->render_content();
            
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            
            // Check if device was registered successfully to close modal
            auto& ts_state = registry_.get_state<TSPanelState>("TSPanel");
            if (ts_state.has_registered_device) {
                state.show_add_device_modal = false;
                ts_state.clear_state();
                // Refresh devices list
                state.fetch_devices();
                ImGui::CloseCurrentPopup();
            }
            
            // Close button
            float button_width = 120.0f;
            float window_width = ImGui::GetWindowWidth();
            float button_x = (window_width - button_width) * 0.5f;
            ImGui::SetCursorPosX(button_x);
            
            if (ImGui::Button("Close", ImVec2(button_width, 36))) {
                state.show_add_device_modal = false;
                ts_state.clear_state();
                ImGui::CloseCurrentPopup();
            }
            
            ImGui::EndPopup();
        }
    }

    void DevicesPanel::show_edit_device_modal(DevicesState& state) {
        if (state.show_edit_device_modal) {
            ImGui::OpenPopup("Edit Device##modal");
        }

        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(420, 300), ImGuiCond_Appearing);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(28.0f, 24.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 12.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12.0f, 10.0f));
        ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.12f, 0.12f, 0.12f, 1.0f));

        if (ImGui::BeginPopupModal("Edit Device##modal", &state.show_edit_device_modal,
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar)) {

            float width = ImGui::GetContentRegionAvail().x;

            // Header
            core::WithFontScale(1.4f, []() {
                core::ColoredText(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Edit Device");
            });
            core::ColoredText(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Update device information");

            ImGui::Spacing();
            ImGui::Spacing();

            // Device Name field
            core::ColoredText(ImVec4(0.85f, 0.85f, 0.85f, 1.0f), "Device Name");
            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.18f, 0.18f, 0.18f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.22f, 0.22f, 0.22f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.25f, 0.25f, 0.25f, 1.0f));
            ImGui::SetNextItemWidth(width);
            ImGui::InputText("##edit_device_name", state.editing_device_name, IM_ARRAYSIZE(state.editing_device_name));
            ImGui::PopStyleColor(3);

            ImGui::Spacing();

            // Mount Path field
            core::ColoredText(ImVec4(0.85f, 0.85f, 0.85f, 1.0f), "Mount Path");
            core::ColoredText(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "(Optional)");
            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.18f, 0.18f, 0.18f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.22f, 0.22f, 0.22f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.25f, 0.25f, 0.25f, 1.0f));
            ImGui::SetNextItemWidth(width);
            ImGui::InputTextWithHint("##edit_mount_path", "/path/to/mount", state.editing_mount_path, IM_ARRAYSIZE(state.editing_mount_path));
            ImGui::PopStyleColor(3);

            ImGui::Spacing();
            ImGui::Spacing();

            // Buttons - right-aligned
            float button_width = 110.0f;
            float button_spacing = 12.0f;
            float total_button_width = (button_width * 2) + button_spacing;
            float button_start_x = width - total_button_width;

            ImGui::SetCursorPosX(button_start_x);

            // Cancel button (secondary style)
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.25f, 0.25f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.35f, 0.35f, 0.35f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
            if (ImGui::Button("Cancel", ImVec2(button_width, 38))) {
                state.show_edit_device_modal = false;
                state.editing_device_id = "";
                state.editing_device_name[0] = '\0';
                state.editing_mount_path[0] = '\0';
                ImGui::CloseCurrentPopup();
            }
            ImGui::PopStyleColor(3);

            ImGui::SameLine(0, button_spacing);

            // Save button (primary style)
            if (core::StyledButton("Save Changes", ImVec2(button_width, 38), core::ButtonTheme::Primary())) {
                // Build JSON object with device information
                std::map<std::string, std::string> json_fields;
                json_fields["device_name"] = std::string(state.editing_device_name);
                json_fields["mount_path"] = std::string(state.editing_mount_path);
                std::string json_body = core::build_json_object(json_fields);

                // Set headers
                std::map<std::string, std::string> headers;
                headers["Content-Type"] = "application/json";

                // Make PUT request to update device
                std::string url = "http://localhost:3000/api/devices?id=" + state.editing_device_id;
                core::HttpResponse response = core::HttpClient::get().put(url, json_body, headers);

                if (response.status_code >= 200 && response.status_code < 300) {
                    state.success_msg = "Device updated successfully";
                    state.show_edit_device_modal = false;
                    state.editing_device_id = "";
                    state.editing_device_name[0] = '\0';
                    state.editing_mount_path[0] = '\0';
                    state.fetch_devices();
                    ImGui::CloseCurrentPopup();
                } else {
                    state.error_msg = "Failed to update device (" + std::to_string(response.status_code) + ")";
                }
            }

            ImGui::EndPopup();
        }

        ImGui::PopStyleColor();
        ImGui::PopStyleVar(4);
    }

    void DevicesPanel::show_delete_confirm_modal(DevicesState& state) {
        if (state.show_delete_confirm) {
            ImGui::OpenPopup("Delete Device##confirm");
        }

        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(420, 260), ImGuiCond_Appearing);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(28.0f, 24.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 12.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
        ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.12f, 0.12f, 0.12f, 1.0f));

        if (ImGui::BeginPopupModal("Delete Device##confirm", &state.show_delete_confirm,
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar)) {

            float width = ImGui::GetContentRegionAvail().x;

            // Header with warning icon
            core::WithFontScale(1.4f, []() {
                core::ColoredText(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Delete Device");
            });

            ImGui::Spacing();
            ImGui::Spacing();

            // Warning box with colored background
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.4f, 0.15f, 0.15f, 0.4f));
            ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16.0f, 14.0f));

            ImGui::BeginChild("WarningBox", ImVec2(width, 90), true);
            {
                core::ColoredText(ImVec4(0.9f, 0.9f, 0.9f, 1.0f), "You are about to delete:");
                ImGui::Spacing();
                core::WithFontScale(1.1f, [&]() {
                    core::ColoredText(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "\"%s\"", state.deleting_device_name.c_str());
                });
                ImGui::Spacing();
                core::ColoredText(ImVec4(0.7f, 0.7f, 0.7f, 1.0f),
                    "This will remove the device from your network.");
                core::ColoredText(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "This action cannot be undone.");
            }
            ImGui::EndChild();

            ImGui::PopStyleVar(2);
            ImGui::PopStyleColor();

            ImGui::Spacing();
            ImGui::Spacing();

            // Buttons - right-aligned
            float button_width = 120.0f;
            float button_spacing = 12.0f;
            float total_button_width = (button_width * 2) + button_spacing;
            float button_start_x = width - total_button_width;

            ImGui::SetCursorPosX(button_start_x);

            // Cancel button (secondary style)
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.25f, 0.25f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.35f, 0.35f, 0.35f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12.0f, 10.0f));
            if (ImGui::Button("Cancel", ImVec2(button_width, 38))) {
                state.show_delete_confirm = false;
                state.deleting_device_id = "";
                state.deleting_device_name = "";
                ImGui::CloseCurrentPopup();
            }
            ImGui::PopStyleVar();
            ImGui::PopStyleColor(3);

            ImGui::SameLine(0, button_spacing);

            // Delete button (danger style)
            if (core::StyledButton("Delete Device", ImVec2(button_width, 38), core::ButtonTheme::Danger())) {
                // Make DELETE request
                std::string url = "http://localhost:3000/api/devices?id=" + state.deleting_device_id;
                core::HttpResponse response = core::HttpClient::get().del(url);

                if (response.status_code >= 200 && response.status_code < 300) {
                    state.success_msg = "Device deleted successfully";
                    state.show_delete_confirm = false;
                    state.deleting_device_id = "";
                    state.deleting_device_name = "";
                    state.fetch_devices();
                    ImGui::CloseCurrentPopup();
                } else {
                    state.error_msg = "Failed to delete device (" + std::to_string(response.status_code) + ")";
                }
            }

            ImGui::EndPopup();
        }

        ImGui::PopStyleColor();
        ImGui::PopStyleVar(3);
    }
}
