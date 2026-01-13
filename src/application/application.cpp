#include "application.h"



namespace minidfs {
    void Application::run() {
        try {
            init_platform();
            init_client();
            init_file_sync();
            init_views();

            
        } catch (const std::exception& e) {
            std::cout << "Exception caught in Application::run: " << e.what() << std::endl;
            cleanup();
        }
        std::cout << "Entering main loop." << std::endl;
        while (is_running()) {
            prepare_frame();
            
            app_view_registry_.render_view();
            render_frame();
        }
        cleanup();
    }

    void Application::init_client() {
        auto channel = grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials());
        client_ = std::make_shared<MiniDFSClient>(channel, "minidfs");
    }

    void Application::init_file_sync() {
        if (!client_) {
            throw std::runtime_error("Client not initialized before initializing FileSync.");
        }

        #ifdef _WIN32
            file_sync_ = std::make_unique<minidfs::FileSyncWin32>(client_);
        #elif defined(__APPLE__)
            file_sync_ = std::make_unique<minidfs::FileSyncMac>(client_);
        #else
            file_sync_ = std::make_unique<minidfs::FileSyncLinux>(client_);
        #endif

        file_sync_->init_sync_resources();
        file_sync_->start_sync();
    }

    void Application::init_views() {
        if (!client_) {
            throw std::runtime_error("Client not initialized before initializing views.");
        }

        app_view_registry_.register_view(ViewID::FileExplorer, 
            std::make_unique<minidfs::FileExplorer::FileExplorerView>(ui_registry_, worker_pool_, client_));
        app_view_registry_.register_view(ViewID::None, 
			nullptr); // Placeholder

        AppViewRegistryController::init(&app_view_registry_);
        AppViewRegistryController::switch_view(ViewID::FileExplorer);
    }

    

   

    
};