#pragma once

#include "panels/panel.h"

#include "core/ui_registry.h"
#include "core/worker_pool.h"

#include "file_sidebar_state.h"
#include "workspace_state.h"
#include "panels/devices/devices_state.h"


namespace minidfs::panel {
    class FileSidebarPanel : public Panel {
    public:
        FileSidebarPanel(core::UIRegistry& registry, core::WorkerPool& worker_pool, std::shared_ptr<MiniDFSClient> client);
        void render();

    private:
        void show_workspace_dropdown(WorkspaceState& workspace_state, float width, float padding);
        void show_home_section(WorkspaceState& workspace_state, DevicesState& devices_state, float width, float padding);
        void show_devices_section(DevicesState& devices_state, float width, float padding);
        void show_create_new(FileSidebarState& state, float width, float padding);
        void show_chooser_modal(FileSidebarState& state);
        void show_create_entry_modal(FileSidebarState& state);
        void show_uploader_modal(FileSidebarState& state);
        void show_upload_progress_modal(FileSidebarState& state);
        void show_ms_login_modal(FileSidebarState& state);
        void show_cloud_section(FileSidebarState& state, float width, float padding);
        void show_quick_access(float width, float padding);
        void show_storage_info(float width, float padding);

        // Upload helpers
        void start_file_upload(FileSidebarState& state, const std::vector<std::string>& file_paths);
        void create_upload_session_and_upload(FileSidebarState& state, const std::string& file_path, const std::string& dest_path);
        void initiate_ms_login(FileSidebarState& state);


    private:
        core::UIRegistry& registry_;
        core::WorkerPool& worker_pool_;
        std::shared_ptr<MiniDFSClient> client_;
    };

}

