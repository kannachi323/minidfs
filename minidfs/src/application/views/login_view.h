#pragma once

#include <memory>

#include "app_view.h"
#include "auth_login_panel.h"
#include "ui_registry.h"

using namespace minidfs::core;
using namespace minidfs::panel;

namespace minidfs::view {
    class LoginView : public AppView {
    public:
        LoginView(UIRegistry& ui_registry);
        ~LoginView() override = default;

        void render() override;
        ViewID get_view_id() override;

    private:
        void init_panels();
    private:
        UIRegistry& ui_registry_;

        std::shared_ptr<AuthLoginPanel> auth_login_panel_;
    };
}
