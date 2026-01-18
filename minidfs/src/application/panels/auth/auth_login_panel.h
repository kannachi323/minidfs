#pragma once

#include "panel.h"
#include "ui_registry.h"
#include "auth_login_state.h"
#include "svg_loader.h"

using namespace minidfs::core;

namespace minidfs::panel {
    class AuthLoginPanel : public panel::Panel {
    public:
        AuthLoginPanel(UIRegistry& registry);
        ~AuthLoginPanel() override = default;
        void render() override;

    private:
        void show_header();
        void show_form_fields(AuthLoginState& state);
        void show_login_button(AuthLoginState& state);
        void show_signup_link();
    private:
        UIRegistry& registry_;
    };
}
