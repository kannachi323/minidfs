#pragma once
#include <string>
#include <mutex>
#include <cstring>
#include "ui_registry.h"
#include "app_view_registry.h"

namespace minidfs::auth {

    struct AuthRegisterState : public core::UIState {
        std::mutex mu;
        
        // Input Buffers
        char full_name[128] = "";
        char email[128] = "";
        char password[64] = "";
        char confirm_password[64] = "";
        bool agree_to_terms = false;

        // UI Logic State
        bool is_submitting = false;
        std::string error_msg = "";
        std::string success_msg = "";
        
        const std::string terms_of_service_path = "assets/terms_of_service.html";
        
        void clear_inputs() {
            memset(full_name, 0, sizeof(full_name));
            memset(email, 0, sizeof(email));
            memset(password, 0, sizeof(password));
            memset(confirm_password, 0, sizeof(confirm_password));
            agree_to_terms = false;
        }

        void handle_create_account() {
            // Switch to FileExplorer view after account creation
            //TODO: Implement account creation logic
            core::AppViewRegistryController::switch_view(view::ViewID::FileExplorer);
        }
    };

}