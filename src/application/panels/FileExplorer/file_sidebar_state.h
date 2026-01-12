#pragma once

#include "ui_registry.h"
#include "file_explorer_state.h"
#include <string>

namespace minidfs::FileExplorer {
    struct FileSidebarState : public UIState {
        // 2. Input Buffers: For "New File" or "New Folder" modals
        char name_buffer[256] = "";
        
        bool show_chooser_modal = false;
        bool show_create_entry_modal = false;
        bool show_uploader_modal = false;
        bool create_is_dir = false;
        
        // 4. Action Feedback
        bool is_performing_action = false;
        std::string status_message = ""; 
    };

    inline void create_file(const std::string& file_path) {
        std::cout << "creating file at: " << file_path << std::endl;
        std::ofstream file(file_path);
        if (!file) {
            // Optional: Handle error or log it
            return;
        }
        file.close();
    }
}