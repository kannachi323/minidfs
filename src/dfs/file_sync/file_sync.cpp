#include "file_sync.h"
#include "dfs/file_manager.h"

namespace minidfs {
    FileSync::FileSync(std::shared_ptr<MiniDFSClient> client)
        : client_(std::move(client)) {
        
    }
    
    void FileSync::on_file_created(const std::string& path, bool is_dir) {
        try {
            const std::string v_path = FileManager::VirtualPath(client_->GetClientMountPath(), path);
            std::cout << "File created: " << v_path << ", is_dir: " << is_dir << std::endl;
            client_->StoreFile(v_path);
        } catch (const std::exception& error) {
            std::cerr << "Error in on_file_created: " << error.what() << std::endl;
            return;
        }
    }

    void FileSync::on_file_removed(const std::string& path, bool is_dir) {
        try {
            const std::string v_path = FileManager::VirtualPath(client_->GetClientMountPath(), path);
            std::cout << "File removed: " << path << ", is_dir: " << is_dir << std::endl;
            client_->RemoveFile(v_path);
        } catch (const std::exception& error) {
            std::cerr << "Error in on_file_removed: " << error.what() << std::endl;
            return;
        }
        
    }
    void FileSync::on_file_modified(const std::string& path, bool is_dir) {
        //TODO: check actual modifications, compare crc, etc
        std::cout << "File modified: " << path << ", is_dir: " << is_dir << std::endl;
        client_->StoreFile(path);
    }
    void FileSync::on_file_renamed(const std::string& path, bool is_dir) {
        //TODO: handle rename properly
        std::cout << "File renamed: " << path << ", is_dir: " << is_dir << std::endl;
    }
    
}
