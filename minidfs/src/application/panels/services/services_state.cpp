#include "services_state.h"
#include "core/env_manager.h"
#include "core/http_client.h"
#include "core/util.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <thread>
#include <set>


namespace minidfs::panel {

    ServicesState::ServicesState() {
        check_connections();
    }

    ServicesState::~ServicesState() = default;

    bool ServicesState::has_ms_tokens() {
        std::lock_guard<std::mutex> lock(mu);
        return !ms_connections.empty();
    }

    void ServicesState::check_connections() {
        std::thread([this]() {
            std::string base = core::EnvManager::get().get("PROXY_SERVICE_URL", "");
            if (base.empty()) {
                return; // Can't check without proxy URL
            }
            std::string user_id = core::EnvManager::get().get("USER_ID", "");
            if (user_id.empty()) {
                return; // Can't check without user_id
            }
            std::string url = base + "/api/ms/token?user_id=" + user_id;
            
            std::map<std::string, std::string> headers;
            headers["Accept"] = "application/json";

            std::cout << "Fetching tokens from " << url << std::endl;
            
            core::HttpResponse response = core::HttpClient::get().get(url, headers);
            
            int loaded_count = 0;
            {
                std::lock_guard<std::mutex> lock(mu);
                if (response.status_code >= 200 && response.status_code < 300) {
                    try {
                        auto json = nlohmann::json::parse(response.body);
                        // Response is an array of tokens
                        if (json.is_array()) {
                            for (const auto& token_obj : json) {
                                std::string token = token_obj.value("access_token", std::string(""));
                                std::string ms_user_id = token_obj.value("ms_user_id", std::string(""));
                                std::string display_name = token_obj.value("display_name", std::string(""));
                                std::string email = token_obj.value("email", std::string(""));

                                if (!ms_user_id.empty()) {
                                    MSConnection conn;
                                    conn.access_token = token;
                                    conn.is_authenticated = !token.empty();
                                    conn.profile.id = ms_user_id;
                                    conn.profile.display_name = display_name;
                                    conn.profile.email = email;
                                    conn.profile.loaded = !display_name.empty() || !email.empty();

                                    auto result = ms_connections.insert(conn);
                                }
                            }
                        } else {
                            error_msg = "Invalid response format: expected array";
                        }
                    } catch (const std::exception& ex) {
                        error_msg = std::string("Failed to parse token response: ") + ex.what();
                    }
                } else {
                    error_msg = "Failed to fetch tokens (" + std::to_string(response.status_code) + ")";
                }
            }
        }).detach();
        
    }

    // Helper to find connection by token (iterates since set is keyed by profile.id)
    std::set<MSConnection>::iterator ServicesState::find_by_token(const std::string& token) {
        for (auto it = ms_connections.begin(); it != ms_connections.end(); ++it) {
            if (it->access_token == token) {
                return it;
            }
        }
        return ms_connections.end();
    }

    // Helper to find connection by ms_user_id (uses set's find since keyed by profile.id)
    std::set<MSConnection>::iterator ServicesState::find_by_ms_user_id(const std::string& ms_user_id) {
        MSConnection search_conn;
        search_conn.profile.id = ms_user_id;
        return ms_connections.find(search_conn);
    }

    bool ServicesState::get_onedrive_card_state(const std::string& ms_user_id, OneDriveCardState& out) {
        std::lock_guard<std::mutex> lock(mu);
        auto it = find_by_ms_user_id(ms_user_id);
        if (it == ms_connections.end()) {
            return false;
        }
        out.profile_loaded = it->profile.loaded;
        out.fetching = it->fetching_profile;
        out.profile = it->profile;
        out.is_connected = it->is_authenticated && it->has_token();
        out.current_token = it->access_token;

        if (!out.profile_loaded && !out.fetching && out.is_connected) {
            MSConnection conn = *it;
            ms_connections.erase(it);
            conn.fetching_profile = true;
            ms_connections.insert(conn);
            out.should_fetch = true;
        }
        return true;
    }

    void ServicesState::mark_disconnected(const std::string& ms_user_id) {
        std::lock_guard<std::mutex> lock(mu);
        auto it = find_by_ms_user_id(ms_user_id);
        if (it == ms_connections.end()) {
            return;
        }
        // Can't modify set elements directly, need to remove and reinsert
        MSConnection conn = *it;
        ms_connections.erase(it);
        conn.access_token = "";
        conn.is_authenticated = false;
        // Keep profile info so user knows which account to reconnect
        ms_connections.insert(conn);
        error_msg = "Connection lost. Please reconnect.";
        success_msg = "";
    }

    void ServicesState::disconnect_onedrive(const std::string& ms_user_id) {
        {
            std::lock_guard<std::mutex> lock(mu);
            auto it = find_by_ms_user_id(ms_user_id);
            if (it != ms_connections.end()) {
                ms_connections.erase(it);
            }
            error_msg = "";
            success_msg = "";
        }

        std::string app_user_id = core::EnvManager::get().get("USER_ID", "");
        std::string base = core::EnvManager::get().get("PROXY_SERVICE_URL", "");
        if (!ms_user_id.empty() && !app_user_id.empty() && !base.empty()) {
            std::string url = base + "/api/ms/token?user_id=" + app_user_id + "&ms_user_id=" + ms_user_id;
            std::map<std::string, std::string> headers;
            headers["Accept"] = "application/json";
            core::HttpClient::get().del(url, headers);
        }
    }

    void ServicesState::fetch_ms_user_profile(const std::string& token) {
        std::string ms_user_id;
        {
            std::lock_guard<std::mutex> lock(mu);
            auto it = find_by_token(token);
            if (it == ms_connections.end() || it->access_token.empty()) {
                return;
            }
            ms_user_id = it->profile.id;
            // Mark as fetching - need to remove and reinsert to modify
            MSConnection conn = *it;
            ms_connections.erase(it);
            conn.fetching_profile = true;
            ms_connections.insert(conn);
        }

        std::string url = "https://graph.microsoft.com/v1.0/me";
        std::map<std::string, std::string> headers;
        headers["Authorization"] = "Bearer " + token;
        headers["Accept"] = "application/json";

        core::HttpResponse response = core::HttpClient::get().get(url, headers);

        // Handle 401 separately so we can release lock before refresh attempt
        if (response.status_code == 401) {
            // Attempt refresh outside the lock
            if (!ms_user_id.empty()) {
                std::string new_token = refresh_ms_token(ms_user_id);
                if (!new_token.empty()) {
                    // Refresh succeeded - update connection and retry
                    {
                        std::lock_guard<std::mutex> lock(mu);
                        auto it = find_by_ms_user_id(ms_user_id);
                        if (it != ms_connections.end()) {
                            MSConnection conn = *it;
                            ms_connections.erase(it);
                            conn.access_token = new_token;
                            conn.is_authenticated = true;
                            conn.fetching_profile = false;
                            ms_connections.insert(conn);
                            success_msg = "Token refreshed.";
                            error_msg = "";
                        }
                    }
                    // Retry with new token
                    fetch_ms_user_profile(new_token);
                    return;
                }
            }

            // Refresh failed - mark as disconnected but keep profile info
            std::lock_guard<std::mutex> lock(mu);
            auto it = find_by_ms_user_id(ms_user_id);
            if (it != ms_connections.end()) {
                MSConnection conn = *it;
                ms_connections.erase(it);
                conn.access_token = "";
                conn.is_authenticated = false;
                conn.fetching_profile = false;
                // Keep profile info so user knows which account to reconnect
                ms_connections.insert(conn);
            }
            error_msg = "Session expired. Please reconnect.";
            success_msg = "";
            return;
        }

        std::lock_guard<std::mutex> lock(mu);
        auto it = find_by_ms_user_id(ms_user_id);
        if (it == ms_connections.end()) {
            return;
        }

        // Update fetching status
        MSConnection conn = *it;
        ms_connections.erase(it);
        conn.fetching_profile = false;

        if (response.status_code >= 200 && response.status_code < 300) {
            try {
                auto json = nlohmann::json::parse(response.body);
                conn.profile.display_name = json.value("displayName", std::string(""));
                conn.profile.email = json.value("mail", std::string(""));
                conn.profile.user_principal_name = json.value("userPrincipalName", std::string(""));
                std::string new_user_id = json.value("id", std::string(""));
                conn.profile.id = new_user_id;
                conn.profile.loaded = true;

                // Use userPrincipalName as fallback for email if mail is empty
                if (conn.profile.email.empty() && !conn.profile.user_principal_name.empty()) {
                    conn.profile.email = conn.profile.user_principal_name;
                }
            } catch (const std::exception& ex) {
                std::cerr << "Failed to parse MS user profile: " << ex.what() << std::endl;
            }
        } else {
            std::cerr << "Failed to fetch MS user profile: " << response.status_code << std::endl;
        }
        ms_connections.insert(conn);
    }

    void ServicesState::initiate_ms_login() {
        std::string proxy_url = core::EnvManager::get().get("PROXY_SERVICE_URL", "");
        if (proxy_url.empty()) throw std::runtime_error("PROXY_SERVICE_URL is not set");

        std::string user_id = core::EnvManager::get().get("USER_ID", "");
        if (user_id.empty()) throw std::runtime_error("USER_ID is not set");

        std::string url = proxy_url + "/api/ms/auth?user_id=" + user_id;

        auto& http = core::HttpClient::get();
        auto response = http.get(url);

        if (response.status_code != 200) {
            ms_auth_error = "Failed to get auth URL. Is the proxy running?";
            show_ms_login_modal = true;
            return;
        }
        try {
            auto json_response = nlohmann::json::parse(response.body);
            std::string auth_url = json_response["auth_url"].get<std::string>();
            core::open_file_in_browser(auth_url);
            show_ms_login_modal = true;
            ms_auth_error.clear();
        } catch (const std::exception&) {
            ms_auth_error = "Failed to parse auth response.";
            show_ms_login_modal = true;
        }
    }

    std::string ServicesState::refresh_ms_token(const std::string& ms_user_id) {
        std::string base = core::EnvManager::get().get("PROXY_SERVICE_URL", "");
        if (base.empty()) {
            return "";
        }
        std::string user_id = core::EnvManager::get().get("USER_ID", "");
        if (user_id.empty()) {
            return "";
        }

        std::string url = base + "/api/ms/token/refresh?user_id=" + user_id + "&ms_user_id=" + ms_user_id;

        std::map<std::string, std::string> headers;
        headers["Accept"] = "application/json";

        core::HttpResponse response = core::HttpClient::get().post(url, "", headers);

        if (response.status_code >= 200 && response.status_code < 300) {
            try {
                auto json = nlohmann::json::parse(response.body);
                std::string new_token = json.value("access_token", std::string(""));
                if (!new_token.empty()) {
                    std::cout << "[ServicesState] Token refreshed for ms_user_id: " << ms_user_id << std::endl;
                    return new_token;
                }
            } catch (const std::exception& ex) {
                std::cerr << "[ServicesState] Failed to parse refresh response: " << ex.what() << std::endl;
            }
        } else {
            std::cerr << "[ServicesState] Token refresh failed with status: " << response.status_code << std::endl;
        }

        return "";
    }

}
