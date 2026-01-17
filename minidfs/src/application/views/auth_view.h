#pragma once

#include <memory>

#include "app_view.h"
#include "auth_register_panel.h"
#include "ui_registry.h"

using namespace minidfs::core;
using namespace minidfs::auth;

namespace minidfs::view {
    class AuthView : public AppView {
    public:
        AuthView(UIRegistry& ui_registry);
        ~AuthView() override = default;

        void render() override;
        ViewID get_view_id() override;

    private:
        void init_panels();
    private:
        UIRegistry& ui_registry_;

        std::shared_ptr<AuthRegisterPanel> auth_register_panel_;
    };
}