
#include <filesystem>
#include <chrono>
#include "minidfs_impl.h"

namespace fs = std::filesystem;

MiniDFSImpl::MiniDFSImpl(const std::string& mount_path) {
    file_manager_ = new FileManager(mount_path);
    pubsub_manager_ = new PubSubManager();
    mount_path_ = mount_path;
    version_ = 0;
}

MiniDFSImpl::~MiniDFSImpl() {
    delete file_manager_;
}

// ---------------------------
// Unary RPCs
// ---------------------------
grpc::ServerUnaryReactor* MiniDFSImpl::GetFileStatus(
    grpc::CallbackServerContext* context,
    const minidfs::FileStatusReq* request,
    minidfs::FileInfo* response) 
{
    class Reactor : public grpc::ServerUnaryReactor {
    public:
        Reactor(MiniDFSImpl* service,
            const minidfs::FileStatusReq* req,
            minidfs::FileInfo* res) : service_(service), request_(req), response_(res)
        {
            std::error_code ec;

            if (FileManager::FileExists(service_->mount_path_, request_->file_path()) == false) {
                Finish(grpc::Status(grpc::StatusCode::NOT_FOUND, "File not found"));
                return;
            }

            response_->set_file_path(request_->file_path());
            response_->set_hash(FileManager::GetFileHash(service_->mount_path_, request_->file_path()));
            Finish(grpc::Status::OK);
        }

        void OnDone() override {
            delete this;
        }
    private:
        MiniDFSImpl* service_;
        const minidfs::FileStatusReq* request_;
        minidfs::FileInfo* response_;
    };

    return new Reactor(this, request, response);
}


grpc::ServerUnaryReactor* MiniDFSImpl::ListAllFiles(
    grpc::CallbackServerContext* context,
    const minidfs::ListAllFilesReq* request,
    minidfs::ListAllFilesRes* response)
{
    class Reactor : public grpc::ServerUnaryReactor {
    public:
        Reactor(MiniDFSImpl* service,
                const minidfs::ListAllFilesReq* req,
                minidfs::ListAllFilesRes* res)
            : service_(service), req_(req), res_(res)
        {
            std::error_code ec;
            fs::path full_dir_path = FileManager::ResolvePath(service_->mount_path_, req->dir_path());
            for (auto& entry : fs::recursive_directory_iterator(full_dir_path)) {
                if (!entry.is_regular_file()) continue;

                auto file_info = res_->add_files();
                fs::path rel = fs::relative(entry.path(), full_dir_path);
                file_info->set_file_path(rel.string());
                file_info->set_hash(FileManager::GetFileHash(service_->mount_path_, entry.path().string()));
            }

            Finish(grpc::Status::OK);
            
        }
        void OnDone() override {
            delete this;
        }
    private:
        MiniDFSImpl* service_;
        const minidfs::ListAllFilesReq* req_;
        minidfs::ListAllFilesRes* res_;
    };

    return new Reactor(this, request, response);
}

grpc::ServerUnaryReactor* MiniDFSImpl::GetWriteLock(
    grpc::CallbackServerContext* context,
    const minidfs::WriteLockReq* request,
    minidfs::WriteLockRes* response)
{
    class Reactor final : public grpc::ServerUnaryReactor {
    public:
        Reactor(MiniDFSImpl* service,
                const minidfs::WriteLockReq* req,
                minidfs::WriteLockRes* res)
            : service_(service)
        {
            bool ok = service_->file_manager_->AcquireWriteLock(
                req->client_id(), req->file_path());

            if (ok) {
                res->set_success(true);
                Finish(grpc::Status::OK);
            } else {
                res->set_success(false);
                Finish(grpc::Status(grpc::StatusCode::ABORTED, "File is locked by another client"));
            }
        }

        void OnDone() override {
            delete this;
        }

    private:
        MiniDFSImpl* service_;
    };

    return new Reactor(this, request, response);
}


grpc::ServerUnaryReactor* MiniDFSImpl::DeleteFile(
    grpc::CallbackServerContext* context,
    const minidfs::DeleteFileReq* request,
    minidfs::DeleteFileRes* response)
{
    class Reactor : public grpc::ServerUnaryReactor {
    public:
        Reactor(MiniDFSImpl* service,
                const minidfs::DeleteFileReq* req,
                minidfs::DeleteFileRes* res)
            : service_(service), req_(req), res_(res)
        {
            std::error_code ec;

            if (!FileManager::FileExists(service_->mount_path_, req_->file_path())) {
                Finish(grpc::Status(grpc::StatusCode::NOT_FOUND, "File not found"));
                return;
            }
            bool success = service_->file_manager_->DeleteFile(req->client_id(), req_->file_path());
            if (!success) {
                Finish(grpc::Status(grpc::StatusCode::INTERNAL, "Failed to delete file"));
                return;
            }

            res_->set_success(true);
            Finish(grpc::Status::OK);
        }

        void OnDone() override {
            service_->file_manager_->ReleaseWriteLock(
                req_->client_id(), req_->file_path());
            service_->pubsub_manager_->Publish(
                req_->file_path(), minidfs::FileUpdateType::DELETED);
            delete this;
        }

    private:
        MiniDFSImpl* service_;
        const minidfs::DeleteFileReq* req_;
        minidfs::DeleteFileRes* res_;
    };

    return new Reactor(this, request, response);
}

grpc::ServerReadReactor<minidfs::FileBuffer>* MiniDFSImpl::StoreFile(
    grpc::CallbackServerContext* context,
    minidfs::StoreFileRes* response)
{
    class Reactor : public grpc::ServerReadReactor<minidfs::FileBuffer> {
    public:
        Reactor(MiniDFSImpl* service,
                minidfs::StoreFileRes* res)
            : service_(service), response_(res)
        {
            StartRead(&current_);
        }

        void OnReadDone(bool ok) override {
            if (!ok) {
                service_->file_manager_->WriteFile(current_.client_id(), current_.file_path(), current_.data());  
                response_->set_success(true);
                response_->set_msg("File stored successfully");
                Finish(grpc::Status::OK);
                return;
            }
            buffer_.insert(buffer_.end(), current_.data().begin(), current_.data().end());
            if (buffer_.size() >= BUFFER_MAX_SIZE) {
                service_->file_manager_->WriteFile(current_.client_id(), current_.file_path(), buffer_.data());   
            }
            
            StartRead(&current_);
        }

        void OnDone() override {
            service_->file_manager_->ReleaseWriteLock(
                current_.client_id(), current_.file_path());
            service_->pubsub_manager_->Publish(
                current_.file_path(), minidfs::FileUpdateType::MODIFIED);
            delete this;
        }
    private:
        MiniDFSImpl* service_;
        minidfs::StoreFileRes* response_;
        minidfs::FileBuffer current_;
        std::vector<char> buffer_;
        const size_t BUFFER_MAX_SIZE = 64 * 1024;
    };
    
    return new Reactor(this, response);
}

grpc::ServerWriteReactor<minidfs::FileBuffer>* MiniDFSImpl::FetchFile(
    grpc::CallbackServerContext* context,
    const minidfs::FetchFileReq* request)
{
    class Reactor : public grpc::ServerWriteReactor<minidfs::FileBuffer> {
    public:
        Reactor(MiniDFSImpl* service,
                const std::string mount_path,
                const minidfs::FetchFileReq* req)
            : service_(service), mount_path_(mount_path), req_(req)
        {   
            if (!service_->file_manager_->FileExists(service_->mount_path_, req_->file_path())) {
                Finish(grpc::Status(grpc::StatusCode::NOT_FOUND, "File not found"));
                return;
            }
            offset_ = 0;
            NextWrite();
        }

        void OnWriteDone(bool ok) override {
            if (!ok) {
                Finish(grpc::Status::OK);
                return;
            }
            NextWrite();
        }

        void OnDone() override {
            delete this;
        }

    private:
        void NextWrite() {
            if (service_->file_manager_->ReadFile(
                    req_->file_path(), offset_, CHUNK_SIZE, &buffer_)) {
                size_t bytes_read = buffer_.data().size();
                
                if (bytes_read > 0) {
                    offset_ += bytes_read;
                    StartWrite(&buffer_);
                } else {
                    Finish(grpc::Status::OK);
                }
            } else {
                Finish(grpc::Status::OK); 
            }
        }
        MiniDFSImpl* service_;
        const minidfs::FetchFileReq* req_;
        uint64_t offset_;
        minidfs::FileBuffer buffer_;
        std::string mount_path_;
        const size_t CHUNK_SIZE = 64 * 1024;
    };
    
    return new Reactor(this, mount_path_, request);
}

grpc::ServerWriteReactor<minidfs::FileUpdateRes>* MiniDFSImpl::FileUpdateCallback(
    grpc::CallbackServerContext* context, 
    const minidfs::FileUpdateReq* request)
{
    class Reactor : public grpc::ServerWriteReactor<minidfs::FileUpdateRes>, public IPubSubReactor {
    public:
        Reactor(MiniDFSImpl* service, const std::string& client_id) {
            service_ = service;
            client_id_ = client_id;
        }

        void NotifyUpdate(const std::string& file_path, minidfs::FileUpdateType type) override {
            std::lock_guard<std::mutex> lock(mu_);  
            
            minidfs::FileInfo file_info;
            file_info.set_file_path(file_path);
            file_info.set_hash(FileManager::GetFileHash(service_->mount_path_, file_path));

            minidfs::FileUpdateRes res;
            res.set_type(type);
            res.set_version(service_->LoadVersion());
            res.mutable_file_info()->CopyFrom(file_info);

            queue_.push(res);

            if (queue_.size() == 1) {
                StartWrite(&queue_.front());
            }
        }

        void OnWriteDone(bool ok) override {
            if (!ok) {
                Finish(grpc::Status::OK);
                return;
            }
            
            {
                std::lock_guard<std::mutex> lock(mu_);
                queue_.pop();
                if (!queue_.empty()) {
                    StartWrite(&queue_.front());
                }
            }
            
        }

        void OnDone() override {
            this->service_->pubsub_manager_->Unsubscribe(client_id_, this);
            delete this;
        }

        void OnCancel() override {
            Finish(grpc::Status::CANCELLED);
        }

    private:
        std::string client_id_;
        MiniDFSImpl* service_;
        std::mutex mu_;
        std::queue<minidfs::FileUpdateRes> queue_;
    };

    auto* reactor = new Reactor(this, request->client_id());

    this->pubsub_manager_->Subscribe(request->client_id(), reactor);

    
    return reactor;
}