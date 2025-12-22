#pragma once

#include <grpcpp/grpcpp.h>
#include "proto_src/minidfs.grpc.pb.h"

class MiniDFSImpl final : public minidfs::MiniDFSService::CallbackService {
public:
    explicit MiniDFSImpl(const std::string& mount_path);

    grpc::ServerUnaryReactor* GetFileStatus(
        grpc::CallbackServerContext* context, 
        const minidfs::FileStatusReq* request, 
        minidfs::FileInfo* response
    ) override;

    grpc::ServerUnaryReactor* ListAllFiles(
        grpc::CallbackServerContext* context, 
        const minidfs::ListAllFilesReq* request, 
        minidfs::ListAllFilesRes* response
    ) override;

    grpc::ServerUnaryReactor* GetWriteLock(
        grpc::CallbackServerContext* context, 
        const minidfs::WriteLockReq* request, 
        minidfs::WriteLockRes* response
    ) override;

    grpc::ServerUnaryReactor* DeleteFile(
        grpc::CallbackServerContext* context, 
        const minidfs::DeleteFileReq* request, 
        minidfs::DeleteFileRes* response
    ) override;

    grpc::ServerReadReactor<minidfs::FileBuffer>* StoreFile(
        grpc::CallbackServerContext* context, 
        minidfs::StoreFileRes* response
    ) override;

    // Server Streaming: Client sends one FetchFileReq, Server returns many FileBuffers
    grpc::ServerWriteReactor<minidfs::FileBuffer>* FetchFile(
        grpc::CallbackServerContext* context, 
        const minidfs::FetchFileReq* request
    ) override;

private:
    std::string mount_path;
};