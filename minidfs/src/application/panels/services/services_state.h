#pragma once

#include <string>
#include <mutex>
#include <set>
#include "core/ui_registry.h"

namespace minidfs::panel {

    // Microsoft account profile information from Graph API
    struct MSUserProfile {
        std::string display_name;
        std::string email;
        std::string user_principal_name;
        std::string id;
        bool loaded = false;
    };

    // Represents a single OneDrive connection
    struct MSConnection {
        std::string access_token;
        MSUserProfile profile;
        bool fetching_profile = false;
        bool is_authenticated = false;

        bool has_token() const { return !access_token.empty(); }

        // Comparison operator for std::set - use profile.id (ms_user_id) as primary key
        // This allows us to keep expired connections in the set with their profile info
        bool operator<(const MSConnection& other) const {
            return profile.id < other.profile.id;
        }
    };

    // Snapshot of a single OneDrive connection for UI (panel uses this for rendering)
    struct OneDriveCardState {
        bool profile_loaded = false;
        bool fetching = false;
        bool should_fetch = false;
        bool is_connected = false;
        std::string current_token;
        MSUserProfile profile;
    };

    class ServicesState : public core::UIState {
    public:
        ServicesState();
        ~ServicesState();

        bool has_ms_tokens(); // Check if any connection has a token
        void check_connections(); // Check once on startup for existing connections
        bool get_onedrive_card_state(const std::string& ms_user_id, OneDriveCardState& out);
        void fetch_token(); // Fetch token from proxy and add as new connection
        void fetch_ms_user_profile(const std::string& token); // Fetch user profile for a specific connection
        void mark_disconnected(const std::string& ms_user_id); // Mark a specific connection as disconnected
        void initiate_ms_login();
        void disconnect_onedrive(const std::string& ms_user_id); // Disconnect a specific OneDrive connection
        std::string refresh_ms_token(const std::string& ms_user_id); // Attempt to refresh token, returns new access_token or empty on failure

        // Helper functions to find connections
        std::set<MSConnection>::iterator find_by_token(const std::string& token);
        std::set<MSConnection>::iterator find_by_ms_user_id(const std::string& ms_user_id);

        std::mutex mu;
        std::string error_msg = "";
        std::string success_msg = "";
        std::set<MSConnection> ms_connections; // Multiple OneDrive connections
        bool show_ms_login_modal = false;
        std::string ms_auth_error;
    };
}
