#pragma once

#include <memory>

#include "app_view.h"
#include "ts_panel.h"
#include "ui_registry.h"

using namespace minidfs::core;
using namespace minidfs::panel;

namespace minidfs::view {
    class TSView : public AppView {
    public:
        TSView(UIRegistry& ui_registry);
        ~TSView() override = default;

        void render() override;
        ViewID get_view_id() override;

    private:
        void init_panels();
    private:
        UIRegistry& ui_registry_;

        std::shared_ptr<TSPanel> ts_panel_;
    };
}
