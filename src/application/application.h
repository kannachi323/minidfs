#pragma once
#include "panel.h"
#include "panel_ui.h"
#include "ui_registry.h"
#include "app_view_registry.h"
#include "worker_pool.h"
#include "app_view.h"
#include "file_explorer_view.h"
#include "asset_manager.h"
#include "dfs/file_sync/file_sync.h" 
#include "dfs/file_sync/file_sync_win.h"
#include "dfs/file_sync/file_sync_mac.h"

namespace minidfs {
    class Application {
    public:
		Application() = default;
        ~Application() = default;

        void run();
        void init_client();
        void init_views();
		void init_file_sync();

    protected:
        // Pure virtual functions (the "Interface")
        virtual void init_platform() = 0;
        virtual void prepare_frame() = 0;
        virtual void render_frame() = 0;
        virtual bool is_running() = 0;
        virtual void cleanup() = 0;

    protected:
        UIRegistry ui_registry_;
        AppViewRegistry app_view_registry_;


        WorkerPool worker_pool_;
        std::shared_ptr<MiniDFSClient> client_;

        #ifdef _WIN32
            std::unique_ptr<FileSyncWin32> file_sync_;
        #elif defined(__APPLE__)
            std::unique_ptr<FileSyncMac> file_sync_;
        #elif defined(__linux__)
            std::unique_ptr<FileSyncLinux> file_sync_;
        #endif

    };  
};
