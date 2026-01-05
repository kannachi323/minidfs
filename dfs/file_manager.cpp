#include "file_manager.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <openssl/evp.h>

namespace fs = std::filesystem;

bool FileManager::AcquireWriteLock(const std::string& client_id, const std::string& file_path, bool create) {
    std::unique_lock<std::mutex> lock(file_lock_mu_);
    auto& fl = file_locks_[file_path];
    if (!fl) fl = std::make_unique<FileLock>();

    fl->pending_writers++;
    fl->cv.wait(lock, [&] {
        return fl->readers == 0 && !fl->has_writer;
        });
    fl->pending_writers--;

    auto session = std::make_unique<FileSession>();
    session->client_id = client_id;
    session->is_writer = true;

    auto parent = fs::path(file_path).parent_path();
    if (!parent.empty()) {
        fs::create_directories(parent);
    }

    session->write_handle = std::make_unique<std::fstream>(
        file_path, std::ios::in | std::ios::out | std::ios::binary
    );

    if (!session->write_handle->is_open() && create) {
        session->write_handle->open(file_path, std::ios::in | std::ios::out | std::ios::binary | std::ios::trunc);
    }

    if (!session->write_handle->is_open() && create) return false;

    fl->has_writer = true;
    fl->sessions[client_id] = std::move(session);
    return true;
}

void FileManager::ReleaseWriteLock(const std::string& client_id, const std::string& file_path) {
    std::unique_lock<std::mutex> lock(file_lock_mu_);
    auto it = file_locks_.find(file_path);
    if (it == file_locks_.end()) return;

    FileLock* fl = it->second.get();
    if (fl->sessions.erase(client_id) > 0) {
        fl->has_writer = false;
        fl->cv.notify_all();
    }
}

bool FileManager::AcquireReadLock(const std::string& client_id, const std::string& file_path) {
    std::unique_lock<std::mutex> lock(file_lock_mu_);
    auto& fl = file_locks_[file_path];
    if (!fl) fl = std::make_unique<FileLock>();

    fl->cv.wait(lock, [&] {
        return !fl->has_writer && fl->pending_writers == 0;
        });

    if (!fs::exists(file_path)) return false;

    auto session = std::make_unique<FileSession>();
    session->client_id = client_id;
    session->is_writer = false;
    session->read_handle = std::make_unique<std::ifstream>(file_path, std::ios::binary);

    if (!session->read_handle->is_open()) return false;

    fl->readers++;
    fl->sessions[client_id] = std::move(session);
    return true;
}

void FileManager::ReleaseReadLock(const std::string& client_id, const std::string& file_path) {
    std::unique_lock<std::mutex> lock(file_lock_mu_);
    auto it = file_locks_.find(file_path);
    if (it == file_locks_.end()) return;

    FileLock* fl = it->second.get();
    if (fl->sessions.erase(client_id) > 0) {
        fl->readers--;
        if (fl->readers == 0) {
            fl->cv.notify_all();
        }
    }
}

void FileManager::ReleaseAllLocks() {
    std::lock_guard<std::mutex> lock(file_lock_mu_);
    file_locks_.clear();
}

bool FileManager::WriteFile(const std::string& client_id, const std::string& file_path, uint64_t offset, const void* data, size_t size) {
    std::lock_guard<std::mutex> lock(file_lock_mu_);

    auto it = file_locks_.find(file_path);
    if (it == file_locks_.end()) return false;

    auto sess_it = it->second->sessions.find(client_id);
    if (sess_it == it->second->sessions.end() || !sess_it->second->is_writer) return false;

    auto& handle = sess_it->second->write_handle;
    if (!handle || !handle->is_open()) return false;

    handle->seekp(offset, std::ios::beg);
    handle->write(static_cast<const char*>(data), size);
    handle->flush();

    return !handle->fail();
}

bool FileManager::ReadFile(const std::string& client_id, const std::string& file_path, uint64_t offset, void* out_data, size_t* bytes_read) {
    std::lock_guard<std::mutex> lock(file_lock_mu_);

    auto it = file_locks_.find(file_path);
    if (it == file_locks_.end()) return false;

    auto sess_it = it->second->sessions.find(client_id);
    if (sess_it == it->second->sessions.end() || sess_it->second->is_writer) return false;

    auto& handle = sess_it->second->read_handle;
    if (!handle || !handle->is_open()) return false;

    handle->clear();
    handle->seekg(offset, std::ios::beg);
    handle->read(static_cast<char*>(out_data), CHUNK_SIZE);
    *bytes_read = static_cast<size_t>(handle->gcount());

    return true;
}

FileStatus FileManager::RemoveFile(const std::string& client_id, const std::string& file_path) {
    std::lock_guard<std::mutex> lock(file_lock_mu_);

    auto it = file_locks_.find(file_path);
    if (it == file_locks_.end()) return FileStatus::FILE_LOCKED;

    FileLock* fl = it->second.get();
    auto session_it = fl->sessions.find(client_id);
    if (session_it == fl->sessions.end() || !session_it->second->is_writer) return FileStatus::FILE_LOCKED;
    if (fl->readers > 0) return FileStatus::FILE_LOCKED;

    fl->sessions.clear();

	if (!fs::exists(file_path)) return FileStatus::FILE_NOT_FOUND;

    std::error_code ec;
    bool removed = fs::remove(file_path, ec);
    file_locks_.erase(it);

    if (ec) return FileStatus::FILE_ERROR;
    return removed ? FileStatus::FILE_OK : FileStatus::FILE_ERROR;
}

fs::path FileManager::ResolvePath(const std::string& mount_path, const std::string& file_path) {
    fs::path mount = fs::absolute(mount_path).lexically_normal();
    fs::path full = (mount / fs::path(file_path).relative_path()).lexically_normal();

    auto mount_str = mount.generic_string();
    auto full_str = full.generic_string();

    if (full_str.find(mount_str) != 0) {
        throw std::runtime_error("Directory traversal attack detected!");
    }

    return full;
}

fs::path FileManager::VirtualPath(const std::string& mount_path, const std::string& file_path) {
    fs::path mount = fs::absolute(mount_path).lexically_normal();
    fs::path full = fs::absolute(file_path).lexically_normal();

    if (full == mount) {
        return "";
    }

    auto rel = fs::relative(full, mount);

    if (rel.empty() || rel.string().find("..") == 0) {
        throw std::runtime_error("File is outside of mount path!");
    }

    return rel;
}

std::string FileManager::GetFileHash(const std::string& file_path) {
    std::ifstream file(file_path, std::ios::binary);
    if (!file) return "";

    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
    const EVP_MD* md = EVP_sha256();

    if (EVP_DigestInit_ex(mdctx, md, nullptr) != 1) {
        EVP_MD_CTX_free(mdctx);
        return "";
    }

    char buffer[32768];
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
    std::stringstream ss;
    for (unsigned int i = 0; i < hash_len; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
}