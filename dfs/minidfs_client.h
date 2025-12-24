#pragma once

#include <string>
#include <memory>
#include <thread>
#include <atomic>
#include <grpcpp/grpcpp.h>
#include "minidfs.grpc.pb.h"
#include "file_manager.h"

class MiniDFSClient {
public:
    explicit MiniDFSClient(std::shared_ptr<grpc::Channel> channel, const std::string& mount_path);
    ~MiniDFSClient();

    grpc::StatusCode GetFileStatus(const std::string& file_path);
    grpc::StatusCode ListAllFiles(const std::string& directory);
    grpc::StatusCode GetWriteLock(const std::string& client_id, const std::string& file_path);
    grpc::StatusCode DeleteFile(const std::string& client_id, const std::string& file_path);
    grpc::StatusCode StoreFile(const std::string& client_id, const std::string& file_path, const std::string& content);
    grpc::StatusCode FetchFile(const std::string& file_path);
    grpc::StatusCode FileUpdateCallback(grpc::ClientContext* sync_context, const std::string& client_id);

    void BeginSync(const std::string& client_id);
    void EndSync();


private:
    grpc::StatusCode ClientStatusCode(const grpc::Status& status);

    
    std::unique_ptr<minidfs::MiniDFSService::Stub> stub_;
    std::string mount_path_;
    std::thread file_update_thread_;
    std::atomic<bool> running_;
    std::unique_ptr<grpc::ClientContext> sync_context_;

    friend class MiniDFSSingleClientTest;
    friend class MiniDFSMultiClientTest;
};
