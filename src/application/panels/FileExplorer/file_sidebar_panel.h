#pragma once

#include "panel.h"

#include "ui_registry.h"
#include "worker_pool.h"

#include "imgui.h"
#include "file_sidebar_state.h"
#include "file_explorer_state.h"
#include "svg_loader.h"

namespace minidfs::FileExplorer {
    class FileSidebarPanel : public Panel {
    public:
        FileSidebarPanel(UIRegistry& registry, WorkerPool& worker_pool, std::shared_ptr<MiniDFSClient> client);
        void render();

    private:
        void show_create_new(FileSidebarState& state, float width, float padding);
        void show_chooser_modal(FileSidebarState& state);
        void show_create_entry_modal(FileSidebarState& state);
        void show_quick_access(float width, float padding);
        void show_storage_info(float width, float padding);
    

    private:
        UIRegistry& registry_;
        WorkerPool& worker_pool_;
        std::shared_ptr<MiniDFSClient> client_;
    };

}

