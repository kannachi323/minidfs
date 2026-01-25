#pragma once
#include <string>
#include <mutex>
#include <cstring>
#include <map>
#include "core/ui_registry.h"
#include "views/app_view.h"
#include "core/http_client.h"
#include "core/util.h"

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

        void validate_inputs() {
            if (strlen(email) == 0 || strlen(password) == 0) {
                error_msg = "Please fill in all fields";
                return;
            }
            error_msg = "";
        }

        void handle_login() {
            validate_inputs();
            if (!error_msg.empty()) {
                return;
            }
            
            is_submitting = true;
            success_msg = "";
            
            // Build JSON object using utility function
            std::map<std::string, std::string> json_fields;
            json_fields["email"] = std::string(email);
            json_fields["password"] = std::string(password);
            std::string json_body = core::build_json_object(json_fields);
            
            // Set headers
            std::map<std::string, std::string> headers;
            headers["Content-Type"] = "application/json";
            
            // Make HTTP POST request
            auto response = core::HttpClient::get().post(
                "http://localhost:3000/api/login",
                json_body,
                headers
            );
            
            is_submitting = false;
            
            // Handle response
            if (response.status_code == 200 || response.status_code == 201) {
                success_msg = "Login successful!";
                clear_inputs();
                // Switch view after successful login
                view::switch_view(view::ViewID::FileExplorer);
            } else if (response.status_code == 400) {
                error_msg = "Invalid login data: " + response.body;
            } else if (response.status_code == 401) {
                error_msg = "Invalid email or password";
            } else if (response.status_code == 500) {
                error_msg = "Server error: Failed to login";
            } else if (response.status_code == 0) {
                error_msg = "Failed to connect to server. Is the proxy running?";
            } else {
                error_msg = "Login failed (Status: " + std::to_string(response.status_code) + "): " + response.body;
            }
        }
    };

}
