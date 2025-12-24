#include "file_manager.h"

namespace fs = std::filesystem;

FileManager::FileManager(std::string mount_path) {
    mount_path_ = mount_path;
}

bool FileManager::AcquireWriteLock(
    const std::string& client_id,
    const std::string& file_path)
{
    std::lock_guard<std::mutex> lock(mu_);
    auto it = write_locks_.find(file_path);
    if (it == write_locks_.end()) {
        write_locks_[file_path] = client_id;
        return true;
    }
    return false;
}

bool FileManager::ReleaseWriteLock(
    const std::string& client_id,
    const std::string& file_path)
{
    std::lock_guard<std::mutex> lock(mu_);
    auto it = write_locks_.find(file_path);
    if (it != write_locks_.end() && it->second == client_id) {
        write_locks_.erase(it);
        return true;
    }

    return false;
}

bool FileManager::WriteFile(
    const std::string& client_id,
    const std::string& file_path,
    const std::string& data)
{
    fs::path full_path = ResolvePath(mount_path_, file_path);

    {
        std::lock_guard<std::mutex> lock(mu_);
        auto it = write_locks_.find(file_path);
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

bool FileManager::ReadFile(const std::string& file_path, uint64_t offset,
                            size_t chunk_size, minidfs::FileBuffer* buf) 
{
    fs::path full_path = ResolvePath(mount_path_, file_path);
    std::ifstream file(full_path, std::ios::binary);
    if (!file.is_open()) return false;

    file.seekg(offset);
    std::vector<char> tmp(chunk_size);
    file.read(tmp.data(), tmp.size());
    std::streamsize n = file.gcount();
    if (n <= 0) return false;

    buf->set_file_path(file_path);
    buf->set_offset(offset);
    buf->set_data(tmp.data(), n);
    return true;
}


fs::path FileManager::ResolvePath(const std::string& mount_path, const std::string& file_path)
{
    return fs::path(mount_path) / file_path;
}

bool FileManager::FileExists(const std::string& file_path)
{
    return fs::exists(fs::path(file_path));
}

std::string FileManager::GetFileHash(const std::string& full_path) {
    std::ifstream file(full_path, std::ios::binary);
    if (!file) return "";

    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
    const EVP_MD* md = EVP_sha256();

    if (EVP_DigestInit_ex(mdctx, md, nullptr) != 1) {
        EVP_MD_CTX_free(mdctx);
        return "";
    }

    char buffer[32768]; // 32KB buffer
    while (file.read(buffer, sizeof(buffer)) || file.gcount() > 0) {
        if (EVP_DigestUpdate(mdctx, buffer, file.gcount()) != 1) {
            EVP_MD_CTX_free(mdctx);
            return "";
        }
    }

    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hash_len = 0;

    if (EVP_DigestFinal_ex(mdctx, hash, &hash_len) != 1) {
        EVP_MD_CTX_free(mdctx);
        return "";
    }

    EVP_MD_CTX_free(mdctx);

    // Convert bytes to hex string
    std::stringstream ss;
    for (unsigned int i = 0; i < hash_len; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
}
