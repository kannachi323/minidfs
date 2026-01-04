#include "file_explorer_panel.h"
#include <iostream>

namespace fs = std::filesystem;

namespace minidfs {
    FileExplorerPanel::FileExplorerPanel(UIRegistry& registry, WorkerPool& worker_pool, std::shared_ptr<MiniDFSClient> client) 
        : registry_(registry), worker_pool_(worker_pool), client_(std::move(client)) {}

    void FileExplorerPanel::render() {
        // Fetch (or create) the state for this panel instance
        auto& state = registry_.get_state<FileExplorerState>("MainExplorer");

        ImGui::Begin("File Explorer");

        // Use the state!
        if (ImGui::Button("Home")) {
            state.current_path = fs::current_path();
        }

        ImGui::SameLine();
        if (ImGui::InputText("Search", state.search_path.data(), state.search_path.capacity() + 1, ImGuiInputTextFlags_EnterReturnsTrue)) {
            state.search_path.resize(strlen(state.search_path.c_str())); 
            state.is_loading = true;
            std::string query = state.search_path; 
            auto client_ptr = client_;
            if (client_ptr == nullptr) {
                state.is_loading = false;
                ImGui::End();
                return;
            }

            worker_pool_.add(
                // TASK
                [client_ptr, query]() {
                    minidfs::ListFilesRes response;
                    grpc::StatusCode status = client_ptr->ListFiles(query, &response);
                 
                    if (status == grpc::StatusCode::NOT_FOUND) {
                        std::cout << "Path not found: " << query << "\n";
                    } else if (status != grpc::StatusCode::OK) {
                        std::cout << "gRPC error" << std::endl;
                    } else {
                        std::cout << "Files in " << query << ":\n";
                        for (const auto& file : response.files()) {
                            std::cout << " - " << file.file_path() << "\n";
                        }
                    }

                },
                // ON_FINISH
                [this, query]() {
                    // 3. Update the ACTUAL state safely via the registry
                    registry_.update_state<FileExplorerState>("MainExplorer", [query](FileExplorerState& s) {
                        s.current_path = fs::path(query);
                        s.is_loading = false;
                    });
                    std::cout << "File listing for " << query << " completed.\n";
                },
                // ON_ERROR
                [this]() {
                    registry_.update_state<FileExplorerState>("MainExplorer", [](FileExplorerState& s) {
                        s.is_loading = false;
                    });
                }
            ); // Don't forget this sem
        }

        ImGui::Separator();

        

        ImGui::End();
    }
};
