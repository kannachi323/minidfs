#include "file_explorer_panel.h"
#include "workspace_state.h"
#include "imgui.h"


namespace fs = std::filesystem;

using namespace minidfs::core;

namespace minidfs::panel {

    FileExplorerPanel::FileExplorerPanel(UIRegistry& registry, WorkerPool& worker_pool, std::shared_ptr<MiniDFSClient> client)
        : registry_(registry), worker_pool_(worker_pool), client_(std::move(client)) {

        auto& state = registry_.get_state<FileExplorerState>("FileExplorer");
        auto& workspace_state = registry_.get_state<WorkspaceState>("Workspace");

        // Fetch workspaces first
        if (!workspace_state.has_fetched) {
            workspace_state.fetch_workspaces();
        }

        // Use workspace mount path if available, otherwise fall back to client mount path
        std::string start_path = workspace_state.get_current_mount_path();
        if (start_path.empty()) {
            start_path = client_->GetClientMountPath();
        }

        // Create directory if it doesn't exist
        if (!start_path.empty()) {
            std::error_code ec;
            fs::create_directories(start_path, ec);
        }

        get_files(state, start_path);
    }

    void FileExplorerPanel::render() {
        auto& state = registry_.get_state<FileExplorerState>("FileExplorer");

        ImGuiWindowFlags file_explorer_flags = ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoResize;

        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.18f, 0.18f, 0.18f, 1.0f));

        if (ImGui::Begin("File Explorer", nullptr, file_explorer_flags)) {
            std::unique_lock<std::mutex> lock(state.mu, std::try_to_lock);

            if (lock.owns_lock()) {
                if (ImGui::BeginChild("TopBar", ImVec2(0, 50), false, ImGuiWindowFlags_NoScrollbar)) {
                    ImGui::SetCursorPosY(8.0f);
                    
                    show_nav_history(state, 30.0f, 8.0f);
                    
                    ImGui::SameLine(0, 8.0f);
                    ImGui::SetCursorPosY(7.0f);

                    
                    show_search_bar(state);
                }
                ImGui::EndChild();

                ImGui::Separator();
                show_directory_contents(state);
                show_error_modal(state.error_msg, "FileExplorerError");
            }
            else {
                ImGui::Text("Syncing...");
            }
        }
        ImGui::End();
        ImGui::PopStyleColor();
    }

    void FileExplorerPanel::show_nav_history(FileExplorerState& state, float button_width, float spacing) {
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6.0f, 6.0f));

        bool can_go_back = !state.back_history.empty();
        if (!can_go_back) ImGui::BeginDisabled();
        if (ImGui::Button("<", ImVec2(button_width, 0))) {
            go_back(state);
        }
        if (!can_go_back) ImGui::EndDisabled();

        ImGui::SameLine(0, spacing);

        bool can_go_forward = !state.forward_history.empty();
        if (!can_go_forward) ImGui::BeginDisabled();
        if (ImGui::Button(">", ImVec2(button_width, 0))) {
            go_forward(state);
        }
        if (!can_go_forward) ImGui::EndDisabled();

        ImGui::PopStyleVar(2);
    }

    void FileExplorerPanel::show_search_bar(FileExplorerState& state) {

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 8));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);

        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.21f, 0.21f, 0.21f, 1.0f));


        float available_width = ImGui::GetContentRegionAvail().x;
        ImGui::SetNextItemWidth(available_width);

        bool entered = ImGui::InputTextWithHint("##search", "Search or enter path...",
            state.search_path,
            sizeof(state.search_path) - 1,
            ImGuiInputTextFlags_EnterReturnsTrue);

        if (entered) {
            get_files(state, state.search_path);
        }

        ImGui::PopStyleColor();
        ImGui::PopStyleVar(2);
    }

    void FileExplorerPanel::show_directory_contents(FileExplorerState& state) {
        static ImGuiTableFlags flags = ImGuiTableFlags_Reorderable | ImGuiTableFlags_Sortable |
            ImGuiTableFlags_Hideable | ImGuiTableFlags_RowBg |
            ImGuiTableFlags_ScrollY | ImGuiTableFlags_Resizable;

        if (ImGui::BeginTable("FileTable", 4, flags)) {
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, 80.0f);
            ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 80.0f);
            ImGui::TableSetupColumn("Last Modified", ImGuiTableColumnFlags_WidthFixed, 150.0f);
            ImGui::TableHeadersRow();

            if (state.files.empty()) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                float column_width = ImGui::GetColumnWidth();

                const char* text = "No files found...";
                float text_width = ImGui::CalcTextSize(text).x;
                float padding = (column_width - text_width) * 0.5f;
                if (padding > 0) {
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + padding);
                }
                ImGui::TextDisabled("%s", text);
            }
            else {
                for (int i = 0; i < (int)state.files.size(); i++) {
                    show_file_item(state, i);
                }
            }

            if (ImGuiTableSortSpecs* sorts_specs = ImGui::TableGetSortSpecs()) {
                if (sorts_specs->SpecsDirty) {
                    sorts_specs->SpecsDirty = false;
                }
            }

            ImGui::EndTable();
        }
    }

    void FileExplorerPanel::show_file_item(FileExplorerState& state, int i) {
        ImGuiIO& io = ImGui::GetIO();
        minidfs::FileInfo file = state.files[i];
        std::string display_name = fs::path(file.file_path()).filename().generic_string();

        bool is_currently_selected = state.selected_files.count(file.file_path()) > 0;

        ImGui::TableNextRow();
        ImGui::TableNextColumn();

        std::string label = (file.is_dir() ? "[DIR] " : "[FILE] ") + display_name;

        if (ImGui::Selectable(label.c_str(), is_currently_selected, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowDoubleClick)) {

            if (io.KeyCtrl) {
                if (is_currently_selected) state.selected_files.erase(file.file_path());
                else state.selected_files.insert(file.file_path());
            }
            else if (io.KeyShift && state.last_selected_index != -1) {
                state.selected_files.clear();
                int start = std::min(state.last_selected_index, i);
                int end = std::max(state.last_selected_index, i);
                for (int j = start; j <= end; j++) state.selected_files.insert(state.files[j].file_path());
            }
            else {
                state.selected_files.clear();
                state.selected_files.insert(file.file_path());
            }
            state.last_selected_index = i;

            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
                if (file.is_dir()) {
                    get_files(state, file.file_path()); // Direct path
                }
                else {
                    open_file(file.file_path()); // Direct path
                }
            }
        }
    }
    
    
}