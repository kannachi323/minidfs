#include "main_view.h"


namespace minidfs::view {
    MainView::MainView(UIRegistry& ui_registry,
        WorkerPool& worker_pool, std::shared_ptr<MiniDFSClient> client) : 
        ui_registry_(ui_registry), worker_pool_(worker_pool), client_(client) {

        init_panels();
    }
  
    void MainView::init_panels() {
        file_explorer_panel_ = std::make_shared<panel::FileExplorerPanel>(ui_registry_, worker_pool_, client_);
        file_sidebar_panel_ = std::make_shared<panel::FileSidebarPanel>(ui_registry_, worker_pool_, client_);
        navbar_panel_ = std::make_shared<panel::NavbarPanel>(ui_registry_);
    }

    view::ViewID MainView::get_view_id() {
        return view::ViewID::FileExplorer;
    }
    
    void MainView::render() {
        ImGuiViewport* viewport = ImGui::GetMainViewport();

        // ---------------------------------------------------------
        // 1. Define Layout Constraints
        // ---------------------------------------------------------
        float navbar_width = 77.0f;    // Vertical bar on the left
        float sidebar_ratio = 0.20f;   // 20% of the remaining space
        float min_sidebar_w = 160.0f;

        // ---------------------------------------------------------
        // 2. Calculate Geometry (Left-to-Right Flow)
        // ---------------------------------------------------------

        // --- Navbar Geometry (Left Vertical Strip) ---
        ImVec2 navbar_pos = viewport->WorkPos;
        ImVec2 navbar_size = ImVec2(navbar_width, viewport->WorkSize.y);

        // --- Sidebar Geometry (Middle Column) ---
        // Takes 20% of the space LEFT OVER after the navbar
        float remaining_x = viewport->WorkSize.x - navbar_width;
        float sidebar_w = std::max(min_sidebar_w, remaining_x * sidebar_ratio);
        float sidebar_h = viewport->WorkSize.y;

        // Position starts after the navbar width
        ImVec2 sidebar_pos = ImVec2(viewport->WorkPos.x + navbar_width, viewport->WorkPos.y);

        // --- Explorer Geometry (Right Column) ---
        float explorer_w = remaining_x - sidebar_w;
        float explorer_h = viewport->WorkSize.y;

        // Position starts after navbar AND sidebar
        ImVec2 explorer_pos = ImVec2(sidebar_pos.x + sidebar_w, viewport->WorkPos.y);

        // ---------------------------------------------------------
        // 3. Render
        // ---------------------------------------------------------

        ImGui::SetNextWindowPos(navbar_pos);
        ImGui::SetNextWindowSize(navbar_size);
        navbar_panel_->render();

        ImGui::SetNextWindowPos(sidebar_pos);
        ImGui::SetNextWindowSize(ImVec2(sidebar_w, sidebar_h));
        file_sidebar_panel_->render();

        ImGui::SetNextWindowPos(explorer_pos);
        ImGui::SetNextWindowSize(ImVec2(explorer_w, explorer_h));
        file_explorer_panel_->render();
    }

    

}