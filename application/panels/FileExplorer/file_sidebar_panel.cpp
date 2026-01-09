#include "file_sidebar_panel.h"
#include "panel_ui.h"

namespace minidfs::FileExplorer {
    FileSidebarPanel::FileSidebarPanel(UIRegistry& registry, WorkerPool& worker_pool, std::shared_ptr<MiniDFSClient> client)
        : registry_(registry), worker_pool_(worker_pool), client_(client) {

     
    }

    void FileSidebarPanel::render() {
        auto& state = registry_.get_state<FileSidebarState>("FileSidebar");

        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowViewport(viewport->ID);

        width_ = viewport->WorkSize.x * 0.20f;
        if (width_ < 160.0f) width_ = 160.0f;
        offset_ = 77.0f;
        padding_ = width_ * 0.08f;

       
        ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x + offset_, viewport->WorkPos.y));
        ImGui::SetNextWindowSize(ImVec2(width_, viewport->WorkSize.y));

        ImGuiWindowFlags sidebar_flags = ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoScrollbar;

        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

        if (ImGui::Begin("FileSidebar", nullptr, sidebar_flags)) {
            ImGui::SetCursorPos(ImVec2(padding_, 20));
            
            show_add_item(state);


            ImGui::Dummy(ImVec2(0, 10)); // Responsive spacing

            

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            show_quick_access();

            // Anchored Storage Info (Fixed to bottom)
            show_storage_info();
        }
        ImGui::End();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();
        
    }

    void FileSidebarPanel::show_add_item(FileSidebarState& state) {
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(padding_, 8));


        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.5f, 0.5f, 0.5f, 0.3f));

        float b_width = width_ - (padding_ * 2);
        float b_height = 48.0f;
        
		SVGTexture& plus_icon = AssetManager::get().get_svg_texture("plus-24", 24);
		if (IconButton("##add_file", plus_icon.id, "New", ImVec2(b_width, b_height), 
            nullptr, 18.0f)) {
            
            state.show_new_file_modal = true;
        }
        
        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar(2);
    }

    void FileSidebarPanel::show_quick_access() {
        float content_width = width_ - (padding_ * 2.0f);
        ImGui::SetCursorPosX(padding_);

        ImGui::BeginGroup();

        // --- Sidebar Title ---
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
        ImGui::Text("Quick access");
        ImGui::PopStyleColor();
        ImGui::Spacing();

        // --- Style setup for Sidebar Buttons ---
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 8.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 10.0f));

        // 1. Left-align the button text (x=0.0f, y=0.5f)
        ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.0f, 0.5f));

        // 2. Transparency settings (Matching your "no color" preference)
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 0.3f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.5f, 0.5f, 0.5f, 0.5f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.3f, 0.3f, 0.5f));

        // --- Navigation Buttons ---
        if (ImGui::Button("Recent", ImVec2(content_width, 0))) {
            // Logic for Recent
        }

        if (ImGui::Button("Starred", ImVec2(content_width, 0))) {
            // Logic for Starred
        }

        if (ImGui::Button("Trash", ImVec2(content_width, 0))) {
            // Logic for Trash
        }

        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar(3); // Popping FramePadding, ItemSpacing, AND ButtonTextAlign
        ImGui::EndGroup();
    }

    void FileSidebarPanel::show_storage_info() {
        // Position at bottom (140px from bottom edge)
        ImGui::SetCursorPosY(ImGui::GetWindowHeight() - 140);
        ImGui::SetCursorPosX(padding_);

        float b_width = width_ - (padding_ * 2);

        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10.0f);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.2f, 0.15f, 1.0f));

        // Multi-line text will now wrap or fit within b_width
        if (ImGui::Button("Upload anything\n2.36 GB / 2 GB\nUpgrade", ImVec2(b_width, 80))) {
            // Upgrade Logic
        }

        ImGui::PopStyleColor();
        ImGui::PopStyleVar();
    }
}