#include "file_sidebar_panel.h"
#include "panel_ui.h"
#include "asset_manager.h"

namespace minidfs::FileExplorer {
    FileSidebarPanel::FileSidebarPanel(UIRegistry& registry, WorkerPool& worker_pool, std::shared_ptr<MiniDFSClient> client)
        : registry_(registry), worker_pool_(worker_pool), client_(client) {


    }

    void FileSidebarPanel::render() {
        auto& state = registry_.get_state<FileSidebarState>("FileSidebar");

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

            show_create_new(state, width, padding);

            ImGui::Separator();

            show_quick_access(width, padding);
            show_storage_info(width, padding);

            show_chooser_modal(state);
            show_create_entry_modal(state);
        }

        ImGui::End();
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor();
    }

    void FileSidebarPanel::show_create_new(FileSidebarState& state, float width, float padding) {
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(padding, 8));

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.5f, 0.5f, 0.5f, 0.3f));

        float b_width = width - (padding * 2);
        SVGTexture& plus_icon = AssetManager::get().get_svg_texture("plus-24", 24);

        if (IconButton("##add_file", plus_icon.id, "New", ImVec2(b_width, 48.0f), nullptr, 18.0f)) {
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
}