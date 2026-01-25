#pragma once

#include <string>
#include <vector>
#include <mutex>
#include "core/ui_registry.h"
#include "core/http_client.h"
#include <nlohmann/json.hpp>

namespace minidfs::panel {

    struct WorkspaceInfo {
        std::string workspace_id;
        std::string workspace_name;
        std::string mount_path;
    };

    struct WorkspaceState : public core::UIState {
        std::mutex mu;

        // All available workspaces
        std::vector<WorkspaceInfo> workspaces;

        // Currently selected workspace index
        int selected_workspace_index = 0;

        // Fetch state
        bool is_fetching = false;
        bool has_fetched = false;
        std::string error_msg = "";

        // Get currently selected workspace
        WorkspaceInfo* get_current_workspace() {
            std::lock_guard<std::mutex> lock(mu);
            if (workspaces.empty() || selected_workspace_index < 0 ||
                selected_workspace_index >= static_cast<int>(workspaces.size())) {
                return nullptr;
            }
            return &workspaces[selected_workspace_index];
        }

        // Get current workspace's mount point
        std::string get_current_mount_path() {
            auto* ws = get_current_workspace();
            return ws ? ws->mount_path : "";
        }

        // Get current workspace's name
        std::string get_current_workspace_name() {
            auto* ws = get_current_workspace();
            return ws ? ws->workspace_name : "No Workspace";
        }

        // Get current workspace's ID
        std::string get_current_workspace_id() {
            auto* ws = get_current_workspace();
            return ws ? ws->workspace_id : "";
        }

        void fetch_workspaces() {
            if (is_fetching) {
                return;
            }

            is_fetching = true;
            error_msg = "";

            core::HttpResponse response = core::HttpClient::get().get("http://localhost:3000/api/workspaces");
            if (response.status_code >= 200 && response.status_code < 300) {
                try {
                    auto json = nlohmann::json::parse(response.body);

                    std::vector<WorkspaceInfo> new_workspaces;

                    if (json.is_array()) {
                        for (const auto& ws_json : json) {
                            WorkspaceInfo ws;
                            ws.workspace_id = ws_json.value("workspace_id", "");
                            ws.workspace_name = ws_json.value("workspace_name", "");
                            ws.mount_path = ws_json.value("mount_path", "");
                            new_workspaces.push_back(ws);
                        }
                    }

                    {
                        std::lock_guard<std::mutex> lock(mu);
                        workspaces = std::move(new_workspaces);
                    }

                    has_fetched = true;

                } catch (const std::exception& ex) {
                    error_msg = std::string("Failed to parse workspaces JSON: ") + ex.what();
                }
            } else {
                error_msg = "Failed to fetch workspaces (" + std::to_string(response.status_code) + ")";
            }

            is_fetching = false;
        }

        // Select workspace by index and return whether it changed
        bool select_workspace(int index) {
            std::lock_guard<std::mutex> lock(mu);
            if (index < 0 || index >= static_cast<int>(workspaces.size())) {
                return false;
            }
            if (selected_workspace_index != index) {
                selected_workspace_index = index;
                return true;
            }
            return false;
        }
    };

}
