#pragma once

#include "imgui.h"
#include "core/svg_loader.h"

namespace minidfs::panel {
    enum class IconButtonAlignH {
        Left,
        Center,
        Right
    };

    enum class IconButtonAlignV {
        Top,
        Center,
        Bottom
    };

    inline void IconText(const core::SVGTexture& icon, float icon_display_size, const char* text, float x_offset, float y_offset) {
        ImGui::Image(icon.id, ImVec2(icon_display_size, icon_display_size));
        ImGui::SameLine();
        float current_x = ImGui::GetCursorPosX();
        float current_y = ImGui::GetCursorPosY();
        ImGui::SetCursorPos(ImVec2(current_x + x_offset, current_y + y_offset));
        ImGui::Text("%s", text);
    }


    inline bool IconButton(
        const char* id,
        const core::SVGTexture& icon,
        const char* label,
        const ImVec2& button_size,
        ImFont* font = nullptr,
        float icon_size = 16.0f,
        IconButtonAlignH align_h = IconButtonAlignH::Left,
        IconButtonAlignV align_v = IconButtonAlignV::Center
    ) {
        ImGui::PushID(id);

        ImVec2 backup_pos = ImGui::GetCursorScreenPos();
        const ImGuiStyle& style = ImGui::GetStyle();

        if (font) ImGui::PushFont(font);
        bool pressed = ImGui::Button("##btn", button_size);
        if (font) ImGui::PopFont();

        // Calculate icon position based on alignment
        ImVec2 icon_pos = backup_pos;
        float icon_text_spacing = 8.0f;
        float total_content_width = icon_size + icon_text_spacing + ImGui::CalcTextSize(label).x;
        
        // Horizontal alignment for icon
        switch (align_h) {
            case IconButtonAlignH::Left:
                icon_pos.x += style.FramePadding.x;
                break;
            case IconButtonAlignH::Center:
                icon_pos.x += (button_size.x - total_content_width) * 0.5f;
                break;
            case IconButtonAlignH::Right:
                icon_pos.x += button_size.x - total_content_width - style.FramePadding.x;
                break;
        }

        // Vertical alignment for icon
        switch (align_v) {
            case IconButtonAlignV::Top:
                icon_pos.y += style.FramePadding.y;
                break;
            case IconButtonAlignV::Center:
                icon_pos.y += (button_size.y - icon_size) * 0.5f;
                break;
            case IconButtonAlignV::Bottom:
                icon_pos.y += button_size.y - icon_size - style.FramePadding.y;
                break;
        }

        ImGui::SetCursorScreenPos(icon_pos);
        ImGui::Image(icon.id, { icon_size, icon_size });

        // Calculate text position based on alignment
        if (font) ImGui::PushFont(font);
        ImVec2 txt_size = ImGui::CalcTextSize(label);
        ImVec2 txt_pos = backup_pos;
        
        // Horizontal alignment for text (relative to icon position)
        switch (align_h) {
            case IconButtonAlignH::Left:
                txt_pos.x = icon_pos.x + icon_size + icon_text_spacing;
                break;
            case IconButtonAlignH::Center:
                txt_pos.x = icon_pos.x + icon_size + icon_text_spacing;
                break;
            case IconButtonAlignH::Right:
                txt_pos.x = icon_pos.x + icon_size + icon_text_spacing;
                break;
        }
        
        // Vertical alignment for text
        switch (align_v) {
            case IconButtonAlignV::Top:
                txt_pos.y += style.FramePadding.y;
                break;
            case IconButtonAlignV::Center:
                txt_pos.y += (button_size.y - txt_size.y) * 0.5f;
                break;
            case IconButtonAlignV::Bottom:
                txt_pos.y += button_size.y - txt_size.y - style.FramePadding.y;
                break;
        }

        ImGui::SetCursorScreenPos(txt_pos);
        ImGui::TextUnformatted(label);
        if (font) ImGui::PopFont();

        ImGui::SetCursorScreenPos(backup_pos);
        ImGui::Dummy(button_size);

        ImGui::PopID();
        return pressed;
    }

    inline bool TextInput(
        const char* label,
        char* buf,
        size_t buf_size,
        ImGuiInputTextFlags flags = 0
    ) {
        ImGui::PushID(label);
        ImGui::TextUnformatted(label);
        ImGui::SameLine();
        bool changed = ImGui::InputText("##input", buf, buf_size, flags);
        ImGui::PopID();
        return changed;
    }
}
