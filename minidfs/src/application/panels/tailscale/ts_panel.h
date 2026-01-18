#pragma once

#include "panel.h"
#include "ui_registry.h"
#include "ts_panel_state.h"
#include "svg_loader.h"

using namespace minidfs::core;

namespace minidfs::panel {
    class TSPanel : public panel::Panel {
    public:
        TSPanel(UIRegistry& registry);
        ~TSPanel() override = default;
        void render() override;

    private:
        void show_header();
        void show_status_message(TSPanelState& state);
        void show_login_url(TSPanelState& state);
        void show_fetch_button(TSPanelState& state);
        void show_open_browser_button(TSPanelState& state);
    private:
        UIRegistry& registry_;
    };
}
