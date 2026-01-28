#include "panels/home/file_sidebar_panel.h"
#include "file_explorer_state.h"
#include "workspace_state.h"
#include "panels/services/services_state.h"
#include "panels/panel_ui.h"
#include "core/asset_manager.h"
#include "core/file_picker.h"
#include <nlohmann/json.hpp>
#include <filesystem>


namespace minidfs::panel {
    FileSidebarPanel::FileSidebarPanel(core::UIRegistry& registry, core::WorkerPool& worker_pool, std::shared_ptr<MiniDFSClient> client)
        : registry_(registry), worker_pool_(worker_pool), client_(client) {
    }

    void FileSidebarPanel::render() {
        auto& state = registry_.get_state<FileSidebarState>("FileSidebar");
        auto& workspace_state = registry_.get_state<WorkspaceState>("Workspace");
        auto& services_state = registry_.get_state<ServicesState>("Services");

        if (!workspace_state.has_fetched && !workspace_state.is_fetching) {
            workspace_state.fetch_workspaces();
        }


        ImGuiWindowFlags flags =
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoScrollbar;

        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16.0f, 16.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 12.0f));

        if (ImGui::Begin("FileSidebar", nullptr, flags)) {
            float width = ImGui::GetWindowWidth();
            float padding = width * 0.08f;

        
            show_workspace_dropdown(workspace_state, width, padding);
            ImGui::Separator();
                
            show_create_new(state, width, padding);
            ImGui::Separator();
            
            show_home_section(workspace_state, services_state, width, padding);
            show_services_section(services_state, width, padding);
            ImGui::Separator();

            show_quick_access(width, padding);
            show_storage_info(width, padding);

            show_chooser_modal(state);
            show_create_entry_modal(state);
            show_uploader_modal(state);
        }

        ImGui::End();
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor();
    }

    void FileSidebarPanel::show_workspace_dropdown(WorkspaceState& workspace_state, float width, float padding) {
        float dropdown_width = width - (padding * 2);
        ImGui::SetCursorPosX(padding);

        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12.0f, 8.0f));
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));

        ImGui::SetNextItemWidth(dropdown_width);

        // Build combo label with current workspace name
        std::string preview = workspace_state.get_current_workspace_name();

        if (ImGui::BeginCombo("##workspace_select", preview.c_str(), ImGuiComboFlags_None)) {
            for (size_t i = 0; i < workspace_state.workspaces.size(); ++i) {
                bool is_selected = (workspace_state.selected_workspace_index == (int)i);
                if (ImGui::Selectable(workspace_state.workspaces[i].workspace_name.c_str(), is_selected)) {
                    if (workspace_state.select_workspace((int)i)) {
                        // Workspace changed - navigate file explorer to new mount path
                        auto& file_explorer_state = registry_.get_state<FileExplorerState>("FileExplorer");
                        std::string mount_path = workspace_state.get_current_mount_path();
                        if (!mount_path.empty()) {
                            // Create directory if it doesn't exist
                            std::error_code ec;
                            fs::create_directories(mount_path, ec);
                            get_files(file_explorer_state, mount_path);
                        }
                    }
                }
                if (is_selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar(2);
    }

    void FileSidebarPanel::show_home_section(WorkspaceState& workspace_state, ServicesState& services_state, float width, float padding) {
        float content_width = width - (padding * 2);
        ImGui::SetCursorPosX(padding);

        ImGui::BeginGroup();

        // Home label
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
        ImGui::Text("Home");
        ImGui::PopStyleColor();

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 8.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 4.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.0f, 0.5f));

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.25f, 0.25f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.35f, 0.35f, 0.35f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));

        // Home button - navigates to workspace mount_path
        std::string home_label = "Workspace Root";
        if (ImGui::Button(home_label.c_str(), ImVec2(content_width, 32))) {
            std::string mount_path = workspace_state.get_current_mount_path();
            if (!mount_path.empty()) {
                auto& file_explorer_state = registry_.get_state<FileExplorerState>("FileExplorer");
                std::error_code ec;
                fs::create_directories(mount_path, ec);
                get_files(file_explorer_state, mount_path);
            }
        }

        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar(3);
        ImGui::EndGroup();
        
        // Small spacing before devices section (no separator - reduced spacing)
        ImGui::Dummy(ImVec2(0.0f, 4.0f));
    }
    
    void FileSidebarPanel::show_services_section(ServicesState& services_state, float width, float padding) {
        float content_width = width - (padding * 2);
        ImGui::SetCursorPosX(padding);

        ImGui::BeginGroup();

        // Check if we have cloud services (assume connected if we have token)
        // Note: has_ms_tokens() already acquires the mutex internally
        bool has_cloud_services = services_state.has_ms_tokens();

        if (has_cloud_services) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
            ImGui::Text("Services");
            ImGui::PopStyleColor();

            float max_height = 200.0f;
            float item_height = 28.0f;
            float actual_height = std::min(max_height, item_height + 8.0f);

            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.18f, 0.18f, 0.18f, 1.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 6.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 8.0f));

            if (ImGui::BeginChild("##services_list", ImVec2(content_width, actual_height), true)) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.22f, 0.22f, 0.22f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.32f, 0.32f, 0.32f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.18f, 0.18f, 0.18f, 1.0f));

                // Show cloud services (OneDrive)
                ImVec4 status_color = ImVec4(0.2f, 0.6f, 0.9f, 1.0f); // Blue for cloud
                std::string onedrive_label = "● OneDrive";
                float button_width = ImGui::GetContentRegionAvail().x;
                if (ImGui::Button(onedrive_label.c_str(), ImVec2(button_width, 24))) {
                    // Could navigate to OneDrive root or show options
                    // For now, just show it's connected
                }

                ImGui::PopStyleColor(3);
            }
            ImGui::EndChild();

            ImGui::PopStyleVar(2);
            ImGui::PopStyleColor();
        }

        ImGui::EndGroup();
        
        ImGui::Spacing();
    }

    void FileSidebarPanel::show_create_new(FileSidebarState& state, float width, float padding) {
        ImGui::SetCursorPosX(padding);
        
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12.0f, 8.0f));

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.5f, 0.5f, 0.5f, 0.3f));

        float b_width = width - (padding * 2);
        core::SVGTexture& plus_icon = core::AssetManager::get().get_svg_texture("plus-24", 24);

        if (IconButton("##add_file", plus_icon, "New", ImVec2(b_width, 48.0f), nullptr, 18.0f)) {
            state.show_chooser_modal = true;
        }

        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar(2);
    }

    void FileSidebarPanel::show_chooser_modal(FileSidebarState& state)
    {
        if (state.show_chooser_modal)
            ImGui::OpenPopup("New");

        ImGuiViewport* vp = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(
            ImVec2(vp->WorkPos.x + 16, vp->WorkPos.y + 16),
            ImGuiCond_Appearing);

        ImGui::SetNextWindowSize(ImVec2(320, 360), ImGuiCond_Appearing);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(24, 24));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(12, 16));

        if (ImGui::BeginPopupModal("New", &state.show_chooser_modal,
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove))
        {
            float w = ImGui::GetContentRegionAvail().x;

            ImGui::TextDisabled("Upload");
            ImGui::Separator();

            if (ImGui::Button("Upload Files", ImVec2(w, 40))) {
                state.show_uploader_modal = true;
                state.show_chooser_modal = false;
            }

            ImGui::TextDisabled("Create");
            ImGui::Separator();

            if (ImGui::Button("Create File", ImVec2(w, 40))) {
                state.create_is_dir = false;
                state.show_create_entry_modal = true;
                state.show_chooser_modal = false;
            }

            if (ImGui::Button("Create Folder", ImVec2(w, 40))) {
                state.create_is_dir = true;
                state.show_create_entry_modal = true;
                state.show_chooser_modal = false;
            }

            ImGui::EndPopup();
        }

        ImGui::PopStyleVar(3);
    }


    void FileSidebarPanel::show_create_entry_modal(FileSidebarState& state) {
        const char* title = state.create_is_dir ? "Create Folder" : "Create File";

        if (state.show_create_entry_modal) {
            ImGui::OpenPopup(title); // Match the title exactly
            state.show_create_entry_modal = false;
        }

        // Centering and Styling
        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, { 0.5f, 0.5f });
        ImGui::SetNextWindowSize({ 420, 190 }, ImGuiCond_Appearing);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 24, 24 });
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 12, 16 });
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 12, 10 });

        if (ImGui::BeginPopupModal(title, nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)) {

            ImGui::Text("%s Name", state.create_is_dir ? "Folder" : "File");
            ImGui::SetNextItemWidth(-1);
            ImGui::InputTextWithHint("##name", "Enter name...", state.name_buffer, IM_ARRAYSIZE(state.name_buffer));

            ImGui::Separator();

            float w = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.5f;

            if (ImGui::Button("Create", { w, 36 }) && state.name_buffer[0]) {
                // Use the pointer we stored in the state to avoid registry deadlocks!
				auto& file_explorer_state = registry_.get_state<FileExplorerState>("FileExplorer");
                fs::path p = fs::path(file_explorer_state.current_path) / state.name_buffer;
                create_file(p.generic_string());

                state.name_buffer[0] = '\0';
                ImGui::CloseCurrentPopup();
            }

            ImGui::SameLine();

            if (ImGui::Button("Cancel", { w, 36 })) {
                state.name_buffer[0] = '\0';
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
        ImGui::PopStyleVar(4);
    }

    void FileSidebarPanel::show_quick_access(float width, float padding) {
        float content_width = width - (padding * 2);
        ImGui::SetCursorPosX(padding);

        ImGui::BeginGroup();

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
        ImGui::Text("Quick access");
        ImGui::PopStyleColor();

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 8.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 8.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.0f, 0.5f));

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 0.3f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.5f, 0.5f, 0.5f, 0.5f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.3f, 0.3f, 0.5f));

        ImGui::Button("Recent", ImVec2(content_width, 0));
        ImGui::Button("Starred", ImVec2(content_width, 0));
        ImGui::Button("Trash", ImVec2(content_width, 0));

        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar(3);
        ImGui::EndGroup();
        
        // Add bottom padding for consistent spacing
        ImGui::Spacing();
    }

    void FileSidebarPanel::show_storage_info(float width, float padding) {
        ImGui::SetCursorPosY(ImGui::GetWindowContentRegionMax().y - 100.0f);
        ImGui::SetCursorPosX(padding);

        float b_width = width - (padding * 2);

        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10.0f);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.2f, 0.15f, 1.0f));

        ImGui::Button("Upload anything\n2.36 GB / 2 GB\nUpgrade", ImVec2(b_width, 80));

        ImGui::PopStyleColor();
        ImGui::PopStyleVar();
    }

    void FileSidebarPanel::show_uploader_modal(FileSidebarState& state) {
        if (!state.show_uploader_modal) return;

        auto& services_state = registry_.get_state<ServicesState>("Services");
        if (!services_state.has_ms_tokens()) {
            state.show_uploader_modal = false;
            state.status_message = "Sign in to OneDrive via Services first.";
            return;
        }

        // Open file picker (this blocks until user selects files or cancels)
        core::FilePickerOptions options;
        options.title = "Select Files to Upload";
        core::FilePickerResult result = core::FilePicker::show_dialog(options);

        state.show_uploader_modal = false;

        if (result.has_selection()) {
            //TODO: upload files to OneDrive
        }
    }
}