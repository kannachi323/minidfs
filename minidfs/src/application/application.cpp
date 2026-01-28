#include "application.h"
#include "views/main_view.h"
#include "views/register_view.h"
#include "views/login_view.h"
#include "views/services_view.h"



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
            
            view::render_current_view();
            render_frame();
        }
        cleanup();
    }

    void Application::init_client() {
        std::string mount_path, channel_address;
        std::ifstream config_file("minidfs.conf");
		if (config_file.is_open()) {
            std::getline(config_file, mount_path);
            std::getline(config_file, channel_address);
            config_file.close();
        } else {
            throw std::runtime_error("Failed to open mount configuration file.");
        }
        if (mount_path.empty() || channel_address.empty()) {
            throw std::runtime_error("Mount path is empty or invalid in configuration file.");
        }
        std::cout << channel_address << std::endl;

        auto channel = grpc::CreateChannel(channel_address, grpc::InsecureChannelCredentials());
        auto deadline = std::chrono::system_clock::now() + std::chrono::seconds(5);
        if (channel->WaitForConnected(deadline)) {
            std::cout << "gRPC Connection Successful!" << std::endl;
        }
        else {
            // This doesn't throw, it just returns false if time runs out
            std::cerr << "gRPC Connection Timeout: Is the server running?" << std::endl;
        }
        client_ = std::make_shared<MiniDFSClient>(channel, 
            fs::path(mount_path).string(), "c47f5011-5c7c-4955-9f90-eed53c09d445");
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

        view::register_view(view::ViewID::FileExplorer, 
            std::make_unique<view::MainView>(ui_registry_, worker_pool_, client_));
        view::register_view(view::ViewID::Auth, std::make_unique<view::RegisterView>(ui_registry_));
        view::register_view(view::ViewID::Login, std::make_unique<view::LoginView>(ui_registry_));
        view::register_view(view::ViewID::Services, std::make_unique<view::ServicesView>(ui_registry_));
        view::switch_view(view::ViewID::FileExplorer);
    }
};