#pragma once
#include <string>
#include <mutex>
#include <cstring>
#include "ui_registry.h"
#include "app_view_registry.h"

namespace minidfs::panel {

    struct AuthLoginState : public core::UIState {
        std::mutex mu;
        
        // Input Buffers
        char email[128] = "";
        char password[64] = "";

        // UI Logic State
        bool is_submitting = false;
        std::string error_msg = "";
        std::string success_msg = "";
        
        void clear_inputs() {
            memset(email, 0, sizeof(email));
            memset(password, 0, sizeof(password));
        }

        void handle_login() {
            // Switch to FileExplorer view after login
            //TODO: Implement login logic
            core::AppViewRegistryController::switch_view(view::ViewID::FileExplorer);
        }
    };

}
