#pragma once

#include <grpcpp/grpcpp.h>
#include <atomic>
#include <queue>
#include "proto_src/minidfs.grpc.pb.h"
#include "dfs/file_manager.h"
#include "pubsub_manager.h"

class MiniDFSImpl final : public minidfs::MiniDFSService::CallbackService {
public:
    explicit MiniDFSImpl(const std::string& mount_path);

    grpc::ServerUnaryReactor* ListFiles(
        grpc::CallbackServerContext* context, 
        const minidfs::ListFilesReq* request, 
        minidfs::ListFilesRes* response) override;

    grpc::ServerUnaryReactor* GetFileLock(
        grpc::CallbackServerContext* context, 
        const minidfs::FileLockReq* request, 
        minidfs::FileLockRes* response) override;

    grpc::ServerUnaryReactor* RemoveFile(
        grpc::CallbackServerContext* context, 
        const minidfs::DeleteFileReq* request, 
        minidfs::DeleteFileRes* response) override;

    grpc::ServerReadReactor<minidfs::FileBuffer>* StoreFile(
        grpc::CallbackServerContext* context, 
        minidfs::StoreFileRes* response) override;

    grpc::ServerWriteReactor<minidfs::FileBuffer>* FetchFile(
        grpc::CallbackServerContext* context, 
        const minidfs::FetchFileReq* request) override;
    
    grpc::ServerWriteReactor<minidfs::FileUpdate>* FileUpdateCallback(
        grpc::CallbackServerContext* context, 
        const minidfs::FileUpdate* request) override;

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
    std::unique_ptr<FileManager> file_manager_;
    std::unique_ptr<PubSubManager> pubsub_manager_;
    std::string mount_path_;
    std::atomic<uint64_t> version_;


    friend class MiniDFSSingleClientTest;
    friend class MiniDFSMultiClientTest;
};