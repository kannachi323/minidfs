#pragma once

#include "panels/panel.h"
#include "core/ui_registry.h"
#include "services_state.h"

using namespace minidfs::core;

namespace minidfs::panel {
    class ServicesPanel : public Panel {
    public:
        ServicesPanel(UIRegistry& registry);
        ~ServicesPanel() override = default;
        void render() override;

    private:
        void show_header();
        void show_cloud_section(ServicesState& state);
        void show_ms_login_modal(ServicesState& state);
        void show_onedrive_profile_card(ServicesState& state, const std::string& ms_user_id);

        // OneDrive card UI sub-views (state lives in ServicesState)
        void show_onedrive_card_header(bool is_connected);
        void show_onedrive_card_profile(const OneDriveCardState& card, const std::string& ms_user_id);
        void show_onedrive_card_actions(ServicesState& state, const std::string& ms_user_id, bool is_connected);

    private:
        UIRegistry& registry_;
    };
}
