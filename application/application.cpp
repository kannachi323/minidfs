#include "application.h"
#include <iostream>


namespace minidfs {

    void Application::run() {
        try {
            init_platform();
            init_client();
            init_layers();
        } catch (const std::exception& e) {
            std::cout << "Exception caught in Application::run: " << e.what() << std::endl;
            cleanup();
        }
        
        while (is_running()) {
            prepare_frame();
            
            for (auto& layer : layers_) {
                layer->render();
            }

            render_frame();
        }

        cleanup();
    }

    void Application::init_layers() {   
        push_layer(std::make_unique<FileExplorerPanel>(registry_, worker_pool_, client_));
    }

    void Application::init_client() {
        auto channel = grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials());
        client_ = std::make_shared<MiniDFSClient>(channel, "client");
    }

    void Application::push_layer(std::unique_ptr<Layer> layer) {
        // emplace_back is great for unique_ptr
        layers_.emplace_back(std::move(layer));
    }
};