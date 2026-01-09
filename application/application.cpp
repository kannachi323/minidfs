#include "application.h"

namespace minidfs {
    void Application::run() {
        try {
            init_platform();
            init_client();

            
            init_views();
            
        } catch (const std::exception& e) {
            std::cout << "Exception caught in Application::run: " << e.what() << std::endl;
            cleanup();
        }

        
        while (is_running()) {
            prepare_frame();
            
            app_view_registry_.render_view();

            render_frame();
            
        }
        
        

        cleanup();
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

    void Application::init_client() {
        auto channel = grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials());
        client_ = std::make_shared<MiniDFSClient>(channel, "minidfs");
    }

    
};