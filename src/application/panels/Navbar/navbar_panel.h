#pragma once
#include <memory>
#include "imgui.h"
#include "ui_registry.h"
#include "app_view_registry.h"
#include "navbar_state.h"
#include "panel.h"
#include "svg_loader.h"


namespace minidfs {
    class NavbarPanel : public Panel {
    public:
        NavbarPanel(UIRegistry& ui_registry);
        ~NavbarPanel() override = default;
        void render() override;


    private:
        void show_nav_item(const char* icon, const char* label, int size, int index, NavbarState& state);
        void show_logo_icon();

    private:
		float nav_width_ = 77.0f;
        UIRegistry& ui_registry_;
        SVGTexture folder_icon_;
    };
}