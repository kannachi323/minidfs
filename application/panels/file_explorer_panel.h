#pragma once

#include <memory>
#include "imgui.h"
#include "registry.h"
#include "layer.h"
#include "worker_pool.h"
#include "minidfs_client.h"

namespace minidfs {
   struct FileExplorerState : public UIState {
        std::string search_path = "";
        std::filesystem::path current_path = "";
        std::vector<std::string> remote_files;
        bool is_loading = false;
        std::string error_msg = "";
   };

    class FileExplorerPanel : public Layer {
    public:
        FileExplorerPanel(UIRegistry& registry, WorkerPool& worker_pool, std::shared_ptr<MiniDFSClient> client);
        ~FileExplorerPanel() override = default;
        void render() override;
    private:
        UIRegistry& registry_;
        WorkerPool& worker_pool_;
        std::shared_ptr<MiniDFSClient> client_;
    };
};
