#pragma once

#include <string>
#include <memory>
#include <grpcpp/grpcpp.h>
#include "minidfs.grpc.pb.h"
#include "file_manager.h"

class MiniDFSClient {
public:
    explicit MiniDFSClient(std::shared_ptr<grpc::Channel> channel, const std::string& mount_path);

    grpc::StatusCode GetFileStatus(const std::string& file_path);
    grpc::StatusCode ListAllFiles(const std::string& directory);
    grpc::StatusCode GetWriteLock(const std::string& client_id, const std::string& file_path);
    grpc::StatusCode DeleteFile(const std::string& file_path);
    grpc::StatusCode StoreFile(const std::string& client_id, const std::string& file_path, const std::string& content);
    grpc::StatusCode FetchFile(const std::string& file_path);
    
    grpc::StatusCode ClientStatusCode(const grpc::Status& status);



private:
    std::unique_ptr<minidfs::MiniDFSService::Stub> stub_;
    std::string mount_path_;
};
