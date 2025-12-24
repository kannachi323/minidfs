#pragma once
#include <mutex>
#include <string>
#include <unordered_map>
#include <filesystem>
#include <fstream>
#include <openssl/evp.h>
#include <iomanip>
#include <sstream>
#include <vector>
#include "proto_src/minidfs.pb.h"


class FileManager {
public:
    explicit FileManager(std::string mount_path);

    bool AcquireWriteLock(const std::string& client_id,
                          const std::string& file_path);

    bool ReleaseWriteLock(const std::string& client_id,
                          const std::string& file_path);

    bool WriteFile(const std::string& client_id,
                   const std::string& file_path,
                   const std::string& data);

    bool ReadFile(const std::string& file_path, uint64_t offset,
                   size_t chunk_size, minidfs::FileBuffer* buf);
    
    static std::filesystem::path ResolvePath(const std::string& mount_path, const std::string& file_path);

    static bool FileExists(const std::string& file_path);

    static std::string GetFileHash(const std::string& full_path);

private:
    std::mutex mu_;
    std::unordered_map<std::string, std::string> write_locks_;
    std::string mount_path_;


    friend class MiniDFSClientTest;
};
