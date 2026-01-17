#pragma once

#include "panel.h"
#include "ui_registry.h"
#include "auth_register_state.h"

using namespace minidfs::core;

namespace minidfs::auth {
    class AuthRegisterPanel : public panel::Panel {
    public:
        AuthRegisterPanel(UIRegistry& registry);
        ~AuthRegisterPanel() override = default;
        void render() override;

    private:
        void show_header();
        void show_form_fields(AuthRegisterState& state);
        void show_terms_checkbox(AuthRegisterState& state);
        void show_create_button(AuthRegisterState& state);
        void show_social_buttons();
        void show_login_link();
        void show_status_bar();
        void open_terms_in_browser();

    private:
        UIRegistry& registry_;
    };
}