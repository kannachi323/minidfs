#pragma once

#include "imgui.h"

inline bool IconButton(
    const char* id,
    ImTextureID icon,
    const char* label,
    const ImVec2& button_size,
    ImFont* font = nullptr,
    float icon_size = 16.0f
) {
    ImGui::PushID(id);

    ImVec2 backup_pos = ImGui::GetCursorScreenPos();

    if (font) ImGui::PushFont(font);
    bool pressed = ImGui::Button("##btn", button_size);
    if (font) ImGui::PopFont();

    ImVec2 icon_pos = backup_pos;
    icon_pos.x += 8.0f;
    icon_pos.y += (button_size.y - icon_size) * 0.5f;

    ImGui::SetCursorScreenPos(icon_pos);
    ImGui::Image(icon, { icon_size, icon_size });

    if (font) ImGui::PushFont(font);
    ImVec2 txt_size = ImGui::CalcTextSize(label);
    ImVec2 txt_pos = backup_pos;
    txt_pos.x += 8.0f + icon_size + 8.0f;
    txt_pos.y += (button_size.y - txt_size.y) * 0.5f;

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
