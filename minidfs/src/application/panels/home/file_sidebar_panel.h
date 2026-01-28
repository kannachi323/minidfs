#pragma once

#include "panels/panel.h"

#include "core/ui_registry.h"
#include "core/worker_pool.h"

#include "file_sidebar_state.h"
#include "workspace_state.h"
#include "panels/services/services_state.h"


namespace minidfs::panel {
    class FileSidebarPanel : public Panel {
    public:
        FileSidebarPanel(core::UIRegistry& registry, core::WorkerPool& worker_pool, std::shared_ptr<MiniDFSClient> client);
        void render();

    private:
        void show_workspace_dropdown(WorkspaceState& workspace_state, float width, float padding);
        void show_home_section(WorkspaceState& workspace_state, ServicesState& services_state, float width, float padding);
        void show_services_section(ServicesState& services_state, float width, float padding);
        void show_create_new(FileSidebarState& state, float width, float padding);
        void show_chooser_modal(FileSidebarState& state);
        void show_create_entry_modal(FileSidebarState& state);
        void show_uploader_modal(FileSidebarState& state);
        void show_upload_progress_modal(FileSidebarState& state);
        void show_quick_access(float width, float padding);
        void show_storage_info(float width, float padding);
        
    private:
        core::UIRegistry& registry_;
        core::WorkerPool& worker_pool_;
        std::shared_ptr<MiniDFSClient> client_;
    };

}

