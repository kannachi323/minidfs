

#ifdef _WIN32
    #include <windows.h>
    #include "windows_app.h"
#elif defined(__APPLE__)
    #include "mac_app.h"
#endif

#include "application.h"
#include "layout.h"
#include "file_explorer_panel.h"
#include <memory>
#include "registry.h"
#include "layer.h"

std::unique_ptr<minidfs::Application> create_application() {
    #ifdef _WIN32
        return std::make_unique<minidfs::WindowsApp>();
    #elif defined(__APPLE__)
        return std::make_unique<minidfs::MacApp>();
    #else
        static_assert(sizeof(void*) == 0, "Unsupported platform");
    #endif
}


int main(int, char**) {
    auto app = create_application();

    app->run();

    return 0;
}

    