#pragma once

#include <string>
#include <memory>
#include <thread>
#include <atomic>
#include <filesystem>
#include <unordered_map>
#include <grpcpp/grpcpp.h>
#include "minidfs.grpc.pb.h"
#include "dfs/file_manager.h"

namespace fs = std::filesystem;

struct ClientFileSession {
    std::atomic<int> refs;
    std::mutex mu;
    std::atomic<int64_t> version;
};

class MiniDFSClient {
public:

    explicit MiniDFSClient(std::shared_ptr<grpc::Channel> channel, const std::string& mount_path, const std::string& client_id);
    ~MiniDFSClient();

    grpc::StatusCode ListFiles(const std::string& path, minidfs::ListFilesRes* response);

    grpc::StatusCode GetReadLock(const std::string& file_path);
    grpc::StatusCode GetWriteLock(const std::string& file_path, bool create);  
    
    grpc::StatusCode RemoveFile(const std::string& file_path);
    grpc::StatusCode StoreFile(const std::string& file_path);
    grpc::StatusCode FetchFile(const std::string& file_path);

    std::string GetClientMountPath() const;
    
private:
    std::shared_ptr<ClientFileSession> AcquireClientFileSession(const std::string& file_path);
    void ReleaseClientFileSession(const std::string& file_path);
    
    std::unique_ptr<minidfs::MiniDFSService::Stub> stub_;
    std::string mount_path_;
    std::string client_id_;

    std::thread file_update_thread_;
    std::unique_ptr<grpc::ClientContext> sync_context_;
    
    std::unordered_map<std::string, std::shared_ptr<ClientFileSession>> file_sessions_;
    std::mutex file_sessions_mutex_;

    
};
