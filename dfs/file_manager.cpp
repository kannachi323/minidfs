#include "file_manager.h"

namespace fs = std::filesystem;

FileManager::FileManager(std::string mount_path) {
    mount_path_ = mount_path;
}

bool FileManager::AcquireWriteLock(
    const std::string& client_id,
    const std::string& filepath)
{
    std::lock_guard<std::mutex> lock(mu_);
    auto it = write_locks_.find(filepath);
    if (it == write_locks_.end()) {
        write_locks_[filepath] = client_id;
        return true;
    }
    return false;
}

bool FileManager::ReleaseWriteLock(
    const std::string& client_id,
    const std::string& filepath)
{
    std::lock_guard<std::mutex> lock(mu_);
    auto it = write_locks_.find(filepath);
    if (it != write_locks_.end() && it->second == client_id) {
        write_locks_.erase(it);
        return true;
    }

    return false;
}

bool FileManager::WriteFile(
    const std::string& client_id,
    const std::string& filepath,
    const std::string& data)
{
    fs::path full_path = ResolvePath(mount_path_, filepath);
    std::cout << "Writing to: " << full_path << std::endl;

    {
        std::lock_guard<std::mutex> lock(mu_);
        auto it = write_locks_.find(filepath);
        if (it == write_locks_.end() || it->second != client_id) {
            return false;
        }
    }

    fs::create_directories(full_path.parent_path());

    std::ofstream file(full_path, std::ios::binary | std::ios::out);

    if (!file.is_open()) {
        return false;
    }

    file.write(data.data(), data.size());
    return true;
}

bool FileManager::ReadFile(const std::string& filepath, uint64_t offset,
                            size_t chunk_size, minidfs::FileBuffer* buf) 
{
    fs::path full_path = ResolvePath(mount_path_, filepath);
    std::ifstream file(full_path, std::ios::binary);
    if (!file.is_open()) return false;

    file.seekg(offset);
    std::vector<char> tmp(chunk_size);
    file.read(tmp.data(), tmp.size());
    std::streamsize n = file.gcount();
    if (n <= 0) return false;

    buf->set_file_path(filepath);
    buf->set_offset(offset);
    buf->set_data(tmp.data(), n);
    return true;
}


fs::path FileManager::ResolvePath(const std::string& mount_path, const std::string& file_path)
{
    return fs::path(mount_path) / file_path;
}

bool FileManager::FileExists(const std::string& filepath)
{
    return fs::exists(fs::path(filepath));
}
