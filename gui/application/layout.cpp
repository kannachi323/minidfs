#include "layout.h"

namespace fs = std::filesystem;

void show_master_dockspace() {
    static bool opt_fullscreen = true;
    static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

    // 1. Setup the parent window flags to be "invisible" and full-screen
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    if (opt_fullscreen) {
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    }

    // 2. Begin the "Background" window
    ImGui::Begin("MasterDockSpace", nullptr, window_flags);
    if (opt_fullscreen) ImGui::PopStyleVar(2);

    // 3. The actual DockSpace ID
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
        ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
    }

    ImGui::End();

}

void show_demo_window() {
    ImGui::ShowDemoWindow();
}

void show_file_explorer() {
    ImGui::Begin("File Explorer");

    // Persist the current directory between frames
    static fs::path currentPath = fs::current_path();

    // --- 1. BREADCRUMBS ---
    // We split the path into parts to make them clickable
    std::vector<fs::path> components;
    for (const auto& p : currentPath) components.push_back(p);

    fs::path accumulatePath;
    for (size_t i = 0; i < components.size(); i++) {
        accumulatePath /= components[i];

        if (i > 0) ImGui::SameLine(0.0f, 2.0f); // Tighten spacing between buttons

        if (ImGui::Button(components[i].string().c_str())) {
            currentPath = accumulatePath; // Navigate to this parent folder
        }

        if (i < components.size() - 1) {
            ImGui::SameLine();
            ImGui::Text(">");
        }
    }

    ImGui::Separator();

    // --- 2. FILE LIST ---
    // Scrollable area for the directory contents
    ImGui::BeginChild("FileRegion");

    // Add a "Go Up" button if we aren't at the root
    if (currentPath.has_parent_path()) {
        if (ImGui::Selectable(".. (Up One Level)", false, ImGuiSelectableFlags_AllowDoubleClick)) {
            currentPath = currentPath.parent_path();
        }
    }

    try {
        for (const auto& entry : fs::directory_iterator(currentPath)) {
            const auto& path = entry.path();
            std::string filename = path.filename().string();

            // Prefix folders with an icon/emoji
            std::string label = entry.is_directory() ? "?? " + filename : "?? " + filename;

            // Using Selectable with DoubleClick logic
            if (ImGui::Selectable(label.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick)) {
                if (ImGui::IsMouseDoubleClicked(0)) {
                    if (entry.is_directory()) {
                        currentPath = path; // Navigate into the folder
                    }
                    else {
                        // Open file logic goes here
                    }
                }
            }
        }
    }
    catch (const fs::filesystem_error& e) {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "Access Denied");
    }

    ImGui::EndChild();
    ImGui::End();
}