#include "minidfs_client.h"
#include <iostream>

MiniDFSClient::MiniDFSClient(std::shared_ptr<grpc::Channel> channel, const std::string& mount_path) {
    stub_ = minidfs::MiniDFSService::NewStub(channel);
    mount_path_ = "storage";
}

grpc::StatusCode MiniDFSClient::GetFileStatus(const std::string& path) {
    minidfs::FileStatusReq request;
    request.set_file_path(path);
    minidfs::FileInfo response;
    grpc::ClientContext context;

    grpc::Status status = stub_->GetFileStatus(&context, request, &response);

    if (status.ok()) {
        std::cout << "File: " << response.file_path() 
                  << " Size: " << response.size() << " bytes\n";
    } else {
        std::cout << "RPC failed: " << status.error_code() 
                  << ": " << status.error_message() << std::endl;
    }

    return ClientStatusCode(status);
}

grpc::StatusCode MiniDFSClient::ListAllFiles(const std::string& directory) {
    minidfs::ListAllFilesReq request;
    request.set_dir_path(directory);

    minidfs::ListAllFilesRes response;
    grpc::ClientContext context;

    grpc::Status status = stub_->ListAllFiles(&context, request, &response);

    if (!status.ok()) {
        std::cerr << "ListAllFiles RPC failed: " 
                  << status.error_code() << ": " 
                  << status.error_message() << std::endl;
        return ClientStatusCode(status);
    }

    for (const auto& file_info : response.files()) {
        std::cout << "File: " << file_info.file_path()
                  << " Size: " << file_info.size() << " bytes\n";
    }

    return ClientStatusCode(status);
}

grpc::StatusCode MiniDFSClient::GetWriteLock(const std::string& client_id, const std::string& file_path) {
    minidfs::WriteLockReq request;
    request.set_client_id(client_id);
    request.set_file_path(file_path);

    minidfs::WriteLockRes response;
    grpc::ClientContext context;

    grpc::Status status = stub_->GetWriteLock(&context, request, &response);

    return ClientStatusCode(status);
}

grpc::StatusCode MiniDFSClient::DeleteFile(const std::string& file_path) {
    minidfs::DeleteFileReq request;
    request.set_file_path(file_path);

    minidfs::DeleteFileRes response;
    grpc::ClientContext context;

    grpc::Status status = stub_->DeleteFile(&context, request, &response);

    if (status.ok() && response.success()) {
        std::cout << "Successfully deleted file " << file_path << std::endl;
    } else {
        std::cerr << "Failed to delete file " << file_path << std::endl;
    }
    
    return ClientStatusCode(status);
}

grpc::StatusCode MiniDFSClient::StoreFile(const std::string& client_id, const std::string& target_path, const std::string& content) {
    // Acquire write lock
    minidfs::WriteLockReq lock_req;
    lock_req.set_client_id(client_id);
    lock_req.set_file_path(target_path);

    minidfs::WriteLockRes lock_res;
    grpc::ClientContext lock_ctx;
    grpc::Status lock_status = stub_->GetWriteLock(&lock_ctx, lock_req, &lock_res);

    if (!lock_status.ok() || !lock_res.success()) {
        std::cerr << "Failed to acquire write lock for " << target_path << std::endl;
        return ClientStatusCode(lock_status);
    }

    grpc::ClientContext context;
    minidfs::StoreFileRes response;
    std::unique_ptr<grpc::ClientWriter<minidfs::FileBuffer>> writer(
        stub_->StoreFile(&context, &response));

    minidfs::FileBuffer chunk;
    chunk.set_client_id(client_id);
    chunk.set_file_path(target_path);
    chunk.set_data(content);

    if (!writer->Write(chunk)) {
        std::cerr << "Stream broken" << std::endl;
    }

    writer->WritesDone();
    grpc::Status status = writer->Finish();

    if (status.ok()) {
        std::cout << "StoreFile Success: " << response.msg() << std::endl;
    } else {
        std::cerr << "StoreFile Failed: " << status.error_message() << std::endl;
    }

    return ClientStatusCode(status);
}

grpc::StatusCode MiniDFSClient::FetchFile(const std::string& file_path) {
    minidfs::FetchFileReq request;
    request.set_file_path(file_path);

    grpc::ClientContext context;
    auto reader = stub_->FetchFile(&context, request);

   
    std::string output_path = FileManager::ResolvePath(mount_path_, file_path);
    std::ofstream outfile(output_path, std::ios::binary);
    if (!outfile.is_open()) {
        return grpc::StatusCode::INTERNAL;
    }

    minidfs::FileBuffer chunk;
    while (reader->Read(&chunk)) {
        outfile.write(chunk.data().c_str(), chunk.data().size());
    }
    outfile.close();

    grpc::Status status = reader->Finish();

    return ClientStatusCode(status);
}

grpc::StatusCode MiniDFSClient::ClientStatusCode(const grpc::Status& status) {
    switch (status.error_code()) {
        case grpc::StatusCode::OK:                  return grpc::StatusCode::OK;
        case grpc::StatusCode::ABORTED:             return grpc::StatusCode::ABORTED;
        case grpc::StatusCode::FAILED_PRECONDITION: return grpc::StatusCode::FAILED_PRECONDITION; // Added
        case grpc::StatusCode::NOT_FOUND:           return grpc::StatusCode::NOT_FOUND;
        case grpc::StatusCode::UNAVAILABLE:         return grpc::StatusCode::UNAVAILABLE; // Highly recommended
        default:                                    return grpc::StatusCode::CANCELLED;
    }
}

