#pragma once
#include <string>
#include <mutex>
#include <cstring>
#include <chrono>
#include "core/ui_registry.h"
#include "core/http_client.h"
#include "core/app_view_registry.h"
#include <nlohmann/json.hpp>

namespace minidfs::panel {

    struct TSPanelState : public core::UIState {
        std::mutex mu;
        
        // Tailscale login URL (fetched from proxy)
        std::string login_url = "";
        std::string status = "";
        bool is_connected = false;
        bool is_polling_status = false;
        std::chrono::steady_clock::time_point last_status_check = {};
        int poll_interval_ms = 2000;
        
        // UI Logic State
        bool is_fetching_url = false;
        bool has_fetched_url = false;
        std::string error_msg = "";
        std::string success_msg = "";
        
        // Proxy endpoint URL (for curl request)
        // User will integrate this, but we'll provide a placeholder
        const std::string proxy_url = "http://localhost:3000/api/ts-status";
        
        void clear_state() {
            login_url = "";
            error_msg = "";
            success_msg = "";
            has_fetched_url = false;
            is_fetching_url = false;
            status = "";
            is_connected = false;
            is_polling_status = false;
            last_status_check = {};
        }

        void handle_fetch_login_url() {
            is_fetching_url = true;
            error_msg = "";
            success_msg = "";

            core::HttpResponse response = core::HttpClient::get().get(proxy_url);
            if (response.status_code >= 200 && response.status_code < 300) {
                try {
                    auto json = nlohmann::json::parse(response.body);
                    if (json.contains("auth_url") && json["auth_url"].is_string()) {
                        login_url = json["auth_url"].get<std::string>();
                        has_fetched_url = true;
                        success_msg = "Login URL fetched.";
                        status = json.value("status", "");
                    } else {
                        error_msg = "Missing auth_url in response.";
                    }
                } catch (const std::exception& ex) {
                    error_msg = std::string("Invalid JSON: ") + ex.what();
                }
            } else {
                error_msg = "Proxy request failed (" + std::to_string(response.status_code) + ")";
            }

            is_fetching_url = false;
        }

        void poll_status_if_needed() {
            if (is_connected) {
                return;
            }

            auto now = std::chrono::steady_clock::now();
            if (last_status_check != std::chrono::steady_clock::time_point{} &&
                std::chrono::duration_cast<std::chrono::milliseconds>(now - last_status_check).count() < poll_interval_ms) {
                return;
            }

            is_polling_status = true;
            error_msg = "";

            core::HttpResponse response = core::HttpClient::get().get(proxy_url);
            if (response.status_code >= 200 && response.status_code < 300) {
                try {
                    auto json = nlohmann::json::parse(response.body);
                    status = json.value("status", "");
                    if (status == "connected") {
                        is_connected = true;
                        success_msg = "Tailscale connected. Returning to app...";
                        core::AppViewRegistryController::switch_view(view::ViewID::FileExplorer);
                    }
                } catch (const std::exception& ex) {
                    error_msg = std::string("Invalid JSON: ") + ex.what();
                }
            } else {
                error_msg = "Proxy request failed (" + std::to_string(response.status_code) + ")";
            }

            last_status_check = now;
            is_polling_status = false;
        }
    };

}
