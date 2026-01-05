#pragma once

#include <mutex>
#include <condition_variable>
#include <string>
#include <unordered_map>
#include <filesystem>
#include <fstream>
#include <openssl/evp.h>
#include <iomanip>
#include <sstream>
#include <vector>
#include "proto_src/minidfs.pb.h"

#define CHUNK_SIZE 40 * 1024

enum class FileStatus {
    FILE_OK,
    FILE_NOT_FOUND,
    FILE_LOCKED,
    FILE_ERROR
};

struct FileSession {
    std::string client_id;
    bool is_writer = false;
    std::unique_ptr<std::fstream> write_handle;
    std::unique_ptr<std::ifstream> read_handle;
};

struct FileLock {
    std::condition_variable cv;
    uint64_t readers = 0;
    uint64_t pending_writers = 0;
    bool has_writer = false;
    std::unordered_map<std::string, std::unique_ptr<FileSession>> sessions;
};


class FileManager {
public:

    bool AcquireWriteLock(const std::string& client_id, const std::string& file_path, bool create);

    void ReleaseWriteLock(const std::string& client_id, const std::string& file_path);

    bool AcquireReadLock(const std::string& client_id, const std::string& file_path);

    void ReleaseReadLock(const std::string& client_id, const std::string& file_path);
    
    bool WriteFile(const std::string& client_id, const std::string &file_path, uint64_t offset, const void* data, size_t size);

    bool ReadFile(const std::string& client_id, const std::string& file_path, uint64_t offset, void* out_data, size_t* bytes_read);

    FileStatus RemoveFile(const std::string& client_id, const std::string& file_path);
    
    static std::filesystem::path ResolvePath(const std::string& mount_path, const std::string& file_path);

    static std::string GetFileHash(const std::string& file_path);

    static std::filesystem::path VirtualPath(const std::string& mount_path, const std::string& file_path);

private:
    void ReleaseAllLocks();
        
    std::mutex file_lock_mu_;
    std::unordered_map<std::string, std::unique_ptr<FileLock>> file_locks_;


    friend class MiniDFSSingleClientTest;
    friend class MiniDFSMultiClientTest;
    friend class MiniDFSFileManagerTest;
};
