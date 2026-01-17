#pragma once

#include <memory>

#include "ui_registry.h"
#include "panel.h"
#include "worker_pool.h"
#include "file_explorer_state.h"


namespace minidfs::panel {
    class FileExplorerPanel : public panel::Panel {
    public:
        FileExplorerPanel(core::UIRegistry& registry, core::WorkerPool& worker_pool, std::shared_ptr<MiniDFSClient> client);
        ~FileExplorerPanel() override = default;
        void render() override;

    private:
        void show_nav_history(panel::FileExplorerState& state, float button_width, float spacing);
        void show_search_bar(panel::FileExplorerState& state);
        void show_directory_contents(panel::FileExplorerState& state);
        void show_file_item(panel::FileExplorerState& state, int i);
        void show_error_modal(panel::FileExplorerState& state);
        
    private:
        core::UIRegistry& registry_;
        core::WorkerPool& worker_pool_;
        std::shared_ptr<MiniDFSClient> client_;

    };
};
