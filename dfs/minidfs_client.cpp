#include "minidfs_client.h"
#include <iostream>
#include <fstream>

MiniDFSClient::MiniDFSClient(std::shared_ptr<grpc::Channel> channel, const std::string& mount_path) {
    stub_ = minidfs::MiniDFSService::NewStub(channel);
    mount_path_ = mount_path;
}

MiniDFSClient::~MiniDFSClient() {
    EndSync();
}

/* =========================
   File update streaming
   ========================= */

void MiniDFSClient::BeginSync(const std::string& client_id) {
    file_update_thread_ = std::thread([this, client_id]() {
        sync_context_ = std::make_unique<grpc::ClientContext>();
        grpc::StatusCode status = FileUpdateCallback(sync_context_.get(), client_id);

        if (status != grpc::StatusCode::OK &&
            status != grpc::StatusCode::CANCELLED) {
            std::cerr << "FileUpdateCallback error: "
                      << static_cast<int>(status) << std::endl;
        }
    });
}

void MiniDFSClient::EndSync() {
    if (sync_context_) {
        sync_context_->TryCancel();
    }
    if (file_update_thread_.joinable()) {
        file_update_thread_.join();
    }
}

/* =========================
   Distributed file lock
   ========================= */

grpc::StatusCode MiniDFSClient::GetReadLock(const std::string& client_id, const std::string& file_path) {
    minidfs::FileLockReq request;
    request.set_client_id(client_id);
    request.set_file_path(file_path);
    request.set_op(minidfs::FileOpType::READ);

    minidfs::FileLockRes response;
    grpc::ClientContext context;

    grpc::Status status = stub_->GetFileLock(&context, request, &response);
    return status.error_code();
}

grpc::StatusCode MiniDFSClient::GetWriteLock(const std::string& client_id, const std::string& file_path, bool create) {
    minidfs::FileLockReq request;
    request.set_client_id(client_id);
    request.set_file_path(file_path);

    if (create) {
        request.set_op(minidfs::FileOpType::WRITE);
    } else {
		request.set_op(minidfs::FileOpType::DEL);
    }
    

    minidfs::FileLockRes response;
    grpc::ClientContext context;

    grpc::Status status = stub_->GetFileLock(&context, request, &response);
    return status.error_code();
}



grpc::StatusCode MiniDFSClient::RemoveFile(const std::string& client_id, const std::string& file_path) {
    grpc::StatusCode lock_status = GetWriteLock(client_id, file_path, false);
    if (lock_status != grpc::StatusCode::OK) {
        return lock_status;
    }

    std::shared_ptr<ClientFileSession> session = AcquireClientFileSession(file_path);
    std::lock_guard<std::mutex> session_lock(session->mu);

    minidfs::DeleteFileReq request;
    request.set_client_id(client_id);
    request.set_file_path(file_path);

    minidfs::DeleteFileRes response;
    grpc::ClientContext context;

    grpc::Status status = stub_->RemoveFile(&context, request, &response);
    ReleaseClientFileSession(file_path);

    return status.error_code();
}

/* =========================
   Store file (WRITE)
   ========================= */

grpc::StatusCode MiniDFSClient::StoreFile(const std::string& client_id, const std::string& file_path) {
    grpc::StatusCode lock_status = GetWriteLock(client_id, file_path, true);
    if (lock_status != grpc::StatusCode::OK) {
        return lock_status;
    }

    std::shared_ptr<ClientFileSession> session = AcquireClientFileSession(file_path);
    std::lock_guard<std::mutex> session_lock(session->mu);

    std::ifstream infile(file_path, std::ios::binary);
    if (!infile) {
        ReleaseClientFileSession(file_path);
        return grpc::StatusCode::NOT_FOUND;
    }

    grpc::ClientContext context;
    minidfs::StoreFileRes response;
    auto writer = stub_->StoreFile(&context, &response);

    char buffer[CHUNK_SIZE];
    uint64_t offset = 0;

    while (infile.read(buffer, sizeof(buffer)) || infile.gcount() > 0) {
        minidfs::FileBuffer chunk;
        chunk.set_client_id(client_id);
        chunk.set_file_path(file_path);
        chunk.set_offset(offset);
        chunk.set_data(buffer, infile.gcount());

        if (!writer->Write(chunk)) break;
        offset += infile.gcount();
    }

    writer->WritesDone();
    infile.close();

    grpc::Status status = writer->Finish();
    ReleaseClientFileSession(file_path);
    return status.error_code();
}

/* =========================
   Fetch file (READ)
   ========================= */

grpc::StatusCode MiniDFSClient::FetchFile(const std::string& client_id, const std::string& file_path) {
    grpc::StatusCode lock_status = GetReadLock(client_id, file_path);
    if (lock_status != grpc::StatusCode::OK) {
        return lock_status;
    }

    std::shared_ptr<ClientFileSession> session = AcquireClientFileSession(file_path);
    std::lock_guard<std::mutex> session_lock(session->mu);

    std::ofstream outfile(file_path, std::ios::binary | std::ios::trunc);
    if (!outfile.is_open()) {
        ReleaseClientFileSession(file_path);
        return grpc::StatusCode::INTERNAL;
    }

    minidfs::FetchFileReq request;
    request.set_file_path(file_path);
    request.set_client_id(client_id);

    grpc::ClientContext context;
    auto reader = stub_->FetchFile(&context, request);

    minidfs::FileBuffer chunk;
    while (reader->Read(&chunk)) {
        outfile.write(chunk.data().data(), chunk.data().size());
    }

    outfile.close();

    grpc::Status status = reader->Finish();
    ReleaseClientFileSession(file_path);
    return status.error_code();
}

/* =========================
   Metadata
   ========================= */
grpc::StatusCode MiniDFSClient::ListFiles(const std::string& path, minidfs::ListFilesRes* response) {
    minidfs::ListFilesReq request;
    request.set_path(path);

    grpc::ClientContext context;

    grpc::Status status = stub_->ListFiles(&context, request, response);
    return status.error_code();
}


/* =========================
   Server push callback
   ========================= */

grpc::StatusCode MiniDFSClient::FileUpdateCallback(
    grpc::ClientContext* context,
    const std::string& client_id)
{
    minidfs::FileUpdate req;
    req.set_client_id(client_id);

    auto reader = stub_->FileUpdateCallback(context, req);
    
    minidfs::FileUpdate res;
    while (reader->Read(&res)) {
        switch (res.type()) {
            case minidfs::FileUpdateType::MODIFIED:
                FetchFile(client_id, res.file_info().file_path());
                break;       
            case minidfs::FileUpdateType::DELETED:
                fs::remove(res.file_info().file_path());
                break;
            default:
                break;
        }
    }

    grpc::Status status = reader->Finish();
    return status.error_code();
}

/* =========================
   Local file session mgmt
   ========================= */

std::shared_ptr<ClientFileSession>
MiniDFSClient::AcquireClientFileSession(const std::string& file_path)
{
    std::lock_guard<std::mutex> lock(file_sessions_mutex_);
    auto& session = file_sessions_[file_path];
    if (!session) {
        session = std::make_shared<ClientFileSession>();
    }
    session->refs++;
    return session;
}

void MiniDFSClient::ReleaseClientFileSession(const std::string& file_path)
{
    std::lock_guard<std::mutex> lock(file_sessions_mutex_);
    auto it = file_sessions_.find(file_path);
    if (it != file_sessions_.end()) {
        if (--it->second->refs == 0) {
            file_sessions_.erase(it);
        }
    }
}