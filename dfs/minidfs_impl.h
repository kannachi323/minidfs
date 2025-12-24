#pragma once

#include <grpcpp/grpcpp.h>
#include <atomic>
#include "proto_src/minidfs.grpc.pb.h"
#include "file_manager.h"
#include "pubsub_manager.h"

class MiniDFSImpl final : public minidfs::MiniDFSService::CallbackService {
public:
    explicit MiniDFSImpl(const std::string& mount_path);
    ~MiniDFSImpl();

    grpc::ServerUnaryReactor* GetFileStatus(
        grpc::CallbackServerContext* context, 
        const minidfs::FileStatusReq* request, 
        minidfs::FileInfo* response) override;

    grpc::ServerUnaryReactor* ListAllFiles(
        grpc::CallbackServerContext* context, 
        const minidfs::ListAllFilesReq* request, 
        minidfs::ListAllFilesRes* response) override;

    grpc::ServerUnaryReactor* GetWriteLock(
        grpc::CallbackServerContext* context, 
        const minidfs::WriteLockReq* request, 
        minidfs::WriteLockRes* response) override;

    grpc::ServerUnaryReactor* DeleteFile(
        grpc::CallbackServerContext* context, 
        const minidfs::DeleteFileReq* request, 
        minidfs::DeleteFileRes* response) override;

    grpc::ServerReadReactor<minidfs::FileBuffer>* StoreFile(
        grpc::CallbackServerContext* context, 
        minidfs::StoreFileRes* response) override;

    grpc::ServerWriteReactor<minidfs::FileBuffer>* FetchFile(
        grpc::CallbackServerContext* context, 
        const minidfs::FetchFileReq* request) override;
    
    grpc::ServerWriteReactor<minidfs::FileUpdateRes>* FileUpdateCallback(
        grpc::CallbackServerContext* context, 
        const minidfs::FileUpdateReq* request) override;

    std::atomic<uint64_t> LoadVersion() const {
        return version_.load();
    }

    void IncrementVersion() {
        version_.fetch_add(1);
    }

    void SetVersion(uint64_t new_version) {
        version_.store(new_version);
    }

private:
    FileManager* file_manager_;
    std::string mount_path_;
    PubSubManager* pubsub_manager_;
    std::atomic<uint64_t> version_;
    
    friend class MiniDFSClientTest;
};