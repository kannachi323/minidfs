#pragma once
#include <string>
#include <mutex>
#include <cstring>
#include "ui_registry.h"

namespace minidfs::panel {

    struct TSPanelState : public core::UIState {
        std::mutex mu;
        
        // Tailscale login URL (fetched from proxy)
        std::string login_url = "";
        
        // UI Logic State
        bool is_fetching_url = false;
        bool has_fetched_url = false;
        std::string error_msg = "";
        std::string success_msg = "";
        
        // Proxy endpoint URL (for curl request)
        // User will integrate this, but we'll provide a placeholder
        const std::string proxy_url = "http://localhost:8080/api/tailscale/login"; // TODO: Configure this
        
        void clear_state() {
            login_url = "";
            error_msg = "";
            success_msg = "";
            has_fetched_url = false;
            is_fetching_url = false;
        }

        void handle_fetch_login_url() {
            // TODO: User will implement the curl request here
            // This should make a curl request to proxy_url and populate login_url
            is_fetching_url = true;
            error_msg = "";
            success_msg = "";
        }
    };

}
