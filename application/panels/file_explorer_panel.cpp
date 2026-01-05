#include "file_explorer_panel.h"
#include <iostream>

namespace fs = std::filesystem;

namespace minidfs {
    FileExplorerPanel::FileExplorerPanel(UIRegistry& registry, WorkerPool& worker_pool, std::shared_ptr<MiniDFSClient> client) 
        : registry_(registry), worker_pool_(worker_pool), client_(std::move(client)) {}

    void FileExplorerPanel::render() {
        auto& state = registry_.get_state<FileExplorerState>("FileExplorer");


        ImGui::Begin("File Explorer");
        
        show_search_bar();

        
        ImGui::Separator();
           
		show_directory_contents();

        if (!state.error_msg.empty()) {
            ImGui::OpenPopup("Error Alert");
        }

        // Always define the popup window layout
        if (ImGui::BeginPopupModal("Error Alert", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "SYSTEM ERROR");
            ImGui::Separator();
            ImGui::TextWrapped("%s", state.error_msg.c_str());
            ImGui::Spacing();

            if (ImGui::Button("OK", ImVec2(120, 0))) {
                // This is crucial: Clear the error message so it doesn't reopen immediately
                registry_.update_state<FileExplorerState>("FileExplorer", [](FileExplorerState& s) {
                    s.error_msg = "";
                    });
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        ImGui::End();
    }

    void FileExplorerPanel::show_search_bar() {
        auto& state = registry_.get_state<FileExplorerState>("FileExplorer");

        if (ImGui::BeginChild("FileExplorerSearchBar", ImVec2(0, 35), false)) {

            if (ImGui::Button("Home")) {
                registry_.update_state<FileExplorerState>("FileExplorer", [](FileExplorerState& s) {
                    std::string path_str = fs::path("/").generic_string();
                    snprintf(s.current_path, sizeof(s.current_path), "%s", path_str.c_str());
                });
            }

            ImGui::SameLine();


            if (ImGui::InputText("Search", state.current_path, sizeof(state.current_path) - 1, ImGuiInputTextFlags_EnterReturnsTrue)) {
                get_files(state.current_path);
            }
        }
        ImGui::EndChild();
    }

    void FileExplorerPanel::show_directory_contents() {
        auto& state = registry_.get_state<FileExplorerState>("FileExplorer");
        ImGuiIO& io = ImGui::GetIO();

        // Table Flags: RowBg adds alternating row colors like Google Drive
        static ImGuiTableFlags flags = ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable |
            ImGuiTableFlags_Hideable | ImGuiTableFlags_RowBg |
            ImGuiTableFlags_BordersOuter | ImGuiTableFlags_ScrollY;

        if (ImGui::BeginTable("FileTable", 4, flags)) {
            // 1. Setup Columns
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, 80.0f);
            ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 80.0f);
			ImGui::TableSetupColumn("Last Modified", ImGuiTableColumnFlags_WidthFixed, 150.0f);
            ImGui::TableHeadersRow();

            for (int i = 0; i < (int)state.files.size(); i++) {
                auto& file = state.files[i];
                std::string full_path = file.file_path(); // Unique ID
                std::string display_name = fs::path(full_path).filename().generic_string();

                // 1. Correct check (Always use full_path)
                bool is_currently_selected = state.selected_files.count(full_path) > 0;

                ImGui::TableNextRow();
                ImGui::TableNextColumn();

                std::string label = (file.is_dir() ? "[DIR] " : "[FILE] ") + display_name;

                if (ImGui::Selectable(label.c_str(), is_currently_selected, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowDoubleClick)) {

                    if (io.KeyCtrl) {
                        if (is_currently_selected) state.selected_files.erase(full_path);
                        else state.selected_files.insert(full_path);
                    }
                    else if (io.KeyShift && state.last_selected_index != -1) {
                        state.selected_files.clear();
                        int start = std::min(state.last_selected_index, i);
                        int end = std::max(state.last_selected_index, i);
                        for (int j = start; j <= end; j++) state.selected_files.insert(state.files[j].file_path());
                    }
                    else {
                        state.selected_files.clear();
                        state.selected_files.insert(full_path); // FIX: Insert full_path
                    }
                    state.last_selected_index = i;

                    // 2. Double click check: Use ImGui::IsItemHovered() + DoubleClicked
                    // This is more reliable inside a Selectable loop
                    if (ImGui::IsMouseDoubleClicked(0) && file.is_dir()) {
                        get_files(full_path);
                    }
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted(file.is_dir() ? "Folder" : "File");
                }
            }
            ImGui::EndTable();
        }
    }

    void FileExplorerPanel::get_files(const std::string& path_query) {
        auto& state = registry_.get_state<FileExplorerState>("FileExplorer");

        if (client_ == nullptr) return; //TODO handle error
        

        state.is_loading = true;
        auto client_ptr = client_;
        auto results = std::make_shared<std::vector<minidfs::FileInfo>>();

        worker_pool_.add(
            [client_ptr, path_query, results]() {
                minidfs::ListFilesRes response;
                // Use the path_query passed from the UI
                grpc::StatusCode status = client_ptr->ListFiles(path_query, &response);

                if (status == grpc::StatusCode::OK || status == grpc::StatusCode::NOT_FOUND) {
                    for (const auto& file : response.files()) {
                        results->push_back(file);
                    }
                    return;
                }
                throw std::runtime_error("gRPC fetch failed");
            },
            [this, results, path_query]() {
                registry_.update_state<FileExplorerState>("FileExplorer", [path_query, results](FileExplorerState& s) mutable {
                    std::cout << "UI Updating for path: " << path_query << std::endl;
                    s.update_files(path_query, std::move(*results));
                });
            },
            [this, results]() {
                registry_.update_state<FileExplorerState>("FileExplorer", [results](FileExplorerState& s) {
                    s.is_loading = false;
                    s.error_msg = "Error: Unable to fetch files at this path";
                });
            }
        );
    }

}


