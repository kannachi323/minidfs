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

    // 1. Capture the starting position
    ImVec2 backup_pos = ImGui::GetCursorScreenPos();

    // 2. Draw the Button (the "Hitbox")
    // Use an empty label "##btn" so ImGui doesn't try to draw text itself
    if (font) ImGui::PushFont(font);
    bool pressed = ImGui::Button("##btn", button_size);
    if (font) ImGui::PopFont();

    // 3. Move the cursor back to the start of the button to draw overlays
    ImVec2 icon_pos = backup_pos;
    icon_pos.x += 8.0f; // Left padding
    icon_pos.y += (button_size.y - icon_size) * 0.5f; // Center vertically

    ImGui::SetCursorScreenPos(icon_pos);
    ImGui::Image(icon, { icon_size, icon_size });

    // 4. Calculate and draw text
    if (font) ImGui::PushFont(font);
    ImVec2 txt_size = ImGui::CalcTextSize(label);
    ImVec2 txt_pos = backup_pos;
    txt_pos.x += 8.0f + icon_size + 8.0f; // Padding + Icon + Gap
    txt_pos.y += (button_size.y - txt_size.y) * 0.5f; // Center vertically

    ImGui::SetCursorScreenPos(txt_pos);
    ImGui::TextUnformatted(label);
    if (font) ImGui::PopFont();

    // 5. Restore cursor to the end of the button so the NEXT widget doesn't overlap
    ImGui::SetCursorScreenPos(backup_pos);
    ImGui::Dummy(button_size);

    ImGui::PopID();
    return pressed;
}
