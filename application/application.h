#pragma once
#include <GLFW/glfw3.h>
#include <vector>
#include <memory>
#include <functional>
#include <string>
#include "imgui.h"
#include "layer.h"
#include "registry.h"
#include "file_explorer_panel.h"
#include "layout.h"
#include "worker_pool.h"
#include "minidfs_client.h"


namespace minidfs {
    class Application {
    public:
        virtual ~Application() = default;

        void run();
        void init_layers();
        void init_client();
        void push_layer(std::unique_ptr<Layer> layer);

    protected:
        // Pure virtual functions (the "Interface")
        virtual void init_platform() = 0;
        virtual void prepare_frame() = 0;
        virtual void render_frame() = 0;
        virtual bool is_running() = 0;
        virtual void cleanup() = 0;

    protected:
        UIRegistry registry_;
        WorkerPool worker_pool_;
        std::shared_ptr<MiniDFSClient> client_;
        std::vector<std::unique_ptr<Layer>> layers_;
    };  
};
