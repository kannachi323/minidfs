
#ifdef _WIN32
#include <windows.h>
#include "windows_app.h"
#endif


#include "layout.h"

#ifdef _WIN32
    #include "windows_app.h"
#elif defined(__APPLE__)
    #include "mac_app.h"
#endif

std::unique_ptr<Application> create_application() {
    #ifdef _WIN32
        return std::make_unique<WindowsApp>();
    #elif defined(__APPLE__)
        return std::make_unique<MacApp>();
    #else
        static_assert(sizeof(void*) == 0, "Unsupported platform");
    #endif
}

int run_app() {
    auto app = create_application();

	if (app->init() != 0) {
        return -1;
    }

	while (app->is_running()) {
		app->prepare_frame();
		
		show_master_dockspace();
		show_demo_window();
		show_file_explorer();
				
		app->render_frame();
	}

    app->cleanup();
    return 0;
}


#ifdef _WIN32
int APIENTRY WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd) {
    return run_app();
}
#else
int main(int, char**) {
    return run_app();
}
#endif
    