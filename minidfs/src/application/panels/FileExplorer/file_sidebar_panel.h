#pragma once

#include "panel.h"

#include "ui_registry.h"
#include "worker_pool.h"

#include "file_sidebar_state.h"


namespace minidfs::panel {
    class FileSidebarPanel : public Panel {
    public:
        FileSidebarPanel(core::UIRegistry& registry, core::WorkerPool& worker_pool, std::shared_ptr<MiniDFSClient> client);
        void render();

    private:
        void show_create_new(FileSidebarState& state, float width, float padding);
        void show_chooser_modal(FileSidebarState& state);
        void show_create_entry_modal(FileSidebarState& state);
        void show_quick_access(float width, float padding);
        void show_storage_info(float width, float padding);
    

    private:
        core::UIRegistry& registry_;
        core::WorkerPool& worker_pool_;
        std::shared_ptr<MiniDFSClient> client_;
    };

}

