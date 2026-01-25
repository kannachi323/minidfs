#pragma once

#include "core/ui_registry.h"
#include <string>
#include <vector>
#include <atomic>
#include <mutex>

namespace minidfs::panel {

    // State for a single file upload
    struct FileUploadProgress {
        std::string file_path;
        std::string file_name;
        size_t file_size = 0;
        size_t bytes_uploaded = 0;
        bool is_complete = false;
        bool has_error = false;
        std::string error_message;
    };

    struct FileSidebarState : public core::UIState {
        // Input Buffers: For "New File" or "New Folder" modals
        char name_buffer[256] = "";

        bool show_chooser_modal = false;
        bool show_create_entry_modal = false;
        bool show_uploader_modal = false;
        bool create_is_dir = false;

        // Action Feedback
        bool is_performing_action = false;
        std::string status_message = "";

        // Upload state
        bool is_uploading = false;
        std::atomic<bool> cancel_upload{false};
        std::vector<FileUploadProgress> upload_queue;
        size_t current_upload_index = 0;
        std::mutex upload_mutex;

        // Microsoft OAuth state
        std::string ms_access_token;
        bool show_ms_login_modal = false;
        bool is_ms_authenticated = false;
        char ms_token_buffer[4096] = "";  // Buffer for pasting token
        std::string ms_auth_error;

        // Helper to get current upload progress (thread-safe)
        FileUploadProgress get_current_upload() {
            std::lock_guard<std::mutex> lock(upload_mutex);
            if (current_upload_index < upload_queue.size()) {
                return upload_queue[current_upload_index];
            }
            return {};
        }

        // Helper to update current upload progress (thread-safe)
        void update_upload_progress(size_t bytes_uploaded) {
            std::lock_guard<std::mutex> lock(upload_mutex);
            if (current_upload_index < upload_queue.size()) {
                upload_queue[current_upload_index].bytes_uploaded = bytes_uploaded;
            }
        }

        // Reset upload state
        void reset_upload() {
            std::lock_guard<std::mutex> lock(upload_mutex);
            is_uploading = false;
            cancel_upload.store(false);
            upload_queue.clear();
            current_upload_index = 0;
        }

        // Check if MS is authenticated
        bool has_ms_token() const {
            return !ms_access_token.empty();
        }
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