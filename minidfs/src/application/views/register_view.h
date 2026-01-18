#pragma once

#include <memory>

#include "app_view.h"
#include "auth_register_panel.h"
#include "ui_registry.h"

using namespace minidfs::core;
using namespace minidfs::panel;

namespace minidfs::view {
    class RegisterView : public AppView {
    public:
        RegisterView(UIRegistry& ui_registry);
        ~RegisterView() override = default;

        void render() override;
        ViewID get_view_id() override;

    private:
        void init_panels();
    private:
        UIRegistry& ui_registry_;

        std::shared_ptr<AuthRegisterPanel> auth_register_panel_;
    };
}
