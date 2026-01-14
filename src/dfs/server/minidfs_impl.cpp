
#include <filesystem>
#include <chrono>
#include "minidfs_impl.h"

namespace fs = std::filesystem;

MiniDFSImpl::MiniDFSImpl(const std::string& mount_path) {
    file_manager_ = std::unique_ptr<FileManager>(new FileManager());
    pubsub_manager_ = std::unique_ptr<PubSubManager>(new PubSubManager());
    mount_path_ = mount_path;
    version_ = 0;
}

grpc::ServerUnaryReactor* MiniDFSImpl::ListFiles(
    grpc::CallbackServerContext* context,
    const minidfs::ListFilesReq* request,
    minidfs::ListFilesRes* response)
{
    class Reactor final: public grpc::ServerUnaryReactor {
    public:
        Reactor(MiniDFSImpl* service, const minidfs::ListFilesReq* req, minidfs::ListFilesRes* res) 
            : service_(service)
        {
            try {
                // VirtualPath will validate and strip mount prefix
                fs::path virtual_path = FileManager::VirtualPath(service_->mount_path_, req->path());

                // Now resolve back to full path
                fs::path dir_path = FileManager::ResolvePath(service_->mount_path_, virtual_path.string());

                std::cout << "Listing files in directory: " << dir_path.generic_string() << std::endl;

                if (!fs::exists(dir_path)) {
                    Finish(grpc::Status(grpc::StatusCode::NOT_FOUND, "Directory not found"));
                    return;
                }
                if (!fs::is_directory(dir_path)) {
                    Finish(grpc::Status(grpc::StatusCode::FAILED_PRECONDITION, "Path is not a directory"));
                    return;
                }

                for (const auto& entry : fs::directory_iterator(dir_path)) {
                    //REMEMBER: add back VIRTUAL paths only, not local entry.path()
                    minidfs::FileInfo* file_info = res->add_files();
                    file_info->set_file_path(FileManager::VirtualPath(service_->mount_path_, entry.path().generic_string()).generic_string());
                    file_info->set_is_dir(entry.is_directory());
                    if (!entry.is_directory()) {
                        file_info->set_hash(FileManager::GetFileHash(entry.path().generic_string()));
                    }
                }
                Finish(grpc::Status::OK);

            }
            catch (const std::invalid_argument& e) {
                Finish(grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, e.what()));
                return;
            }
            catch (const std::exception& e) {
                Finish(grpc::Status(grpc::StatusCode::INTERNAL, e.what()));
                return;
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

grpc::ServerUnaryReactor* MiniDFSImpl::GetFileLock(
    grpc::CallbackServerContext* context,
    const minidfs::FileLockReq* request,
    minidfs::FileLockRes* response)
{
    class Reactor final : public grpc::ServerUnaryReactor {
    public:
        Reactor(MiniDFSImpl* service, const minidfs::FileLockReq* req, minidfs::FileLockRes* res) 
            : service_(service)
        {
            bool ok = false;
            file_path_ = FileManager::ResolvePath(service_->mount_path_, req->file_path());
            client_id_ = req->client_id();
            
            if (req->op() == minidfs::FileOpType::READ) {
                ok = service_->file_manager_->AcquireReadLock(client_id_, file_path_.generic_string());
            } else if (req->op() == minidfs::FileOpType::WRITE) {
                ok = service_->file_manager_->AcquireWriteLock(client_id_, file_path_.generic_string(), true);
            } else if (req->op() == minidfs::FileOpType::DEL) {
				ok = service_->file_manager_->AcquireWriteLock(client_id_, file_path_.generic_string(), false);
            }
            
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
        fs::path file_path_;
        std::string client_id_;
    };

    return new Reactor(this, request, response);
}


grpc::ServerUnaryReactor* MiniDFSImpl::RemoveFile(
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
            file_path_ = FileManager::ResolvePath(service_->mount_path_, req_->file_path());

            FileStatus status = service_->file_manager_->RemoveFile(req->client_id(), file_path_.generic_string());

            switch (status) {
                case FileStatus::FILE_LOCKED:
                    res->set_success(false);
                    Finish(grpc::Status(grpc::StatusCode::ABORTED, "File is locked by another client"));
                    break;
                case FileStatus::FILE_NOT_FOUND:
                    res->set_success(false);
                    Finish(grpc::Status(grpc::StatusCode::NOT_FOUND, "File not found"));
                    break;
                case FileStatus::FILE_ERROR:
                    res->set_success(false);
                    Finish(grpc::Status(grpc::StatusCode::INTERNAL, "File deletion error"));
                    break;
                case FileStatus::FILE_OK:
                    res_->set_success(true);
                    Finish(grpc::Status::OK);
                    break;
                default:
                    res->set_success(false);
                    Finish(grpc::Status(grpc::StatusCode::INTERNAL, "Unknown error"));
                    break;
            }
            
        }

        void OnDone() override {
            service_->file_manager_->ReleaseWriteLock(
                req_->client_id(), file_path_.generic_string());
            service_->pubsub_manager_->Publish(req_->client_id(),
                file_path_.generic_string(), minidfs::FileUpdateType::DELETED);
            delete this;
        }

    private:
        MiniDFSImpl* service_;
        const minidfs::DeleteFileReq* req_;
        minidfs::DeleteFileRes* res_;
        fs::path file_path_;
    };

    return new Reactor(this, request, response);
}

grpc::ServerReadReactor<minidfs::FileBuffer>* MiniDFSImpl::StoreFile(
    grpc::CallbackServerContext* context,
    minidfs::StoreFileRes* response)
{
    class Reactor : public grpc::ServerReadReactor<minidfs::FileBuffer> {
    public:
        Reactor(MiniDFSImpl* service, minidfs::StoreFileRes* res)
            : service_(service), response_(res), offset_(0)
        {
            StartRead(&current_);
        }

        void OnReadDone(bool ok) override {
            if (!ok) {
                response_->set_success(true);
                response_->set_msg("File stored successfully");
                service_->file_manager_->ReleaseWriteLock(client_id_, file_path_.generic_string());
                service_->IncrementVersion();
                service_->pubsub_manager_->Publish(client_id_, file_path_.generic_string(), minidfs::FileUpdateType::MODIFIED);
                
                Finish(grpc::Status::OK);
                return;
            }
            
            if (file_path_.empty()) {
                file_path_ = FileManager::ResolvePath(
                    service_->mount_path_, current_.file_path());
                client_id_ = current_.client_id();
            }

            const char* data = static_cast<const char*>(current_.data().data());
            size_t data_size = current_.data().size();

            bool write_ok = service_->file_manager_->WriteFile(
                current_.client_id(), file_path_.generic_string(), 
                offset_, data, data_size);
            if (!write_ok) {
                Finish(grpc::Status(grpc::StatusCode::DATA_LOSS, "Write failed"));
                return;
            }
            offset_ += data_size;
            StartRead(&current_);
        }   

        void OnDone() override {
            delete this;
        }

    private:
        MiniDFSImpl* service_;
        minidfs::StoreFileRes* response_;
        minidfs::FileBuffer current_;
        uint64_t offset_;
        fs::path file_path_;
        std::string client_id_;
    };
    
    return new Reactor(this, response);
}

grpc::ServerWriteReactor<minidfs::FileBuffer>* MiniDFSImpl::FetchFile(
    grpc::CallbackServerContext* context,
    const minidfs::FetchFileReq* request)
{
    class Reactor : public grpc::ServerWriteReactor<minidfs::FileBuffer> {
    public:
        Reactor(MiniDFSImpl* service, const minidfs::FetchFileReq* req)
            : service_(service), req_(req), offset_(0)
        {   
            file_path_ = FileManager::ResolvePath(
                service_->mount_path_, req_->file_path());
            client_id_ = req_->client_id();
            NextWrite();
        }

        void OnWriteDone(bool ok) override {
            if (!ok) {
                if (!file_path_.empty()) {
                    service_->file_manager_->ReleaseReadLock(client_id_, file_path_.generic_string());
                }
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
            std::vector<char> raw_buf(CHUNK_SIZE);
            size_t bytes_read = 0;
    
            bool read_success = service_->file_manager_->ReadFile(
                client_id_, file_path_.generic_string(), offset_, raw_buf.data(), &bytes_read
            );

            if (!read_success) {
                Finish(grpc::Status(grpc::StatusCode::DATA_LOSS, "File Read Error"));
                return;
            }

            if (bytes_read > 0) {
                if (offset_ == 0) {
                    buffer_.set_file_path(file_path_.generic_string());
                } else {
                    buffer_.clear_file_path();
                }

                buffer_.set_offset(offset_);
                buffer_.set_data(raw_buf.data(), bytes_read);

                offset_ += bytes_read;
                
                StartWrite(&buffer_);
            } else {
                Finish(grpc::Status::OK);
            }
        }

        MiniDFSImpl* service_;
        const minidfs::FetchFileReq* req_;
        fs::path file_path_;
        std::string client_id_;
        uint64_t offset_;
        minidfs::FileBuffer buffer_;
    };
    
    return new Reactor(this, request);
}

grpc::ServerWriteReactor<minidfs::FileUpdate>* MiniDFSImpl::FileUpdateCallback(
    grpc::CallbackServerContext* context, 
    const minidfs::FileUpdate* request)
{
    class Reactor : public grpc::ServerWriteReactor<minidfs::FileUpdate>, public IPubSubReactor {
    public:
        Reactor(MiniDFSImpl* service, const std::string& client_id) {
            service_ = service;
            client_id_ = client_id;
        }

        void NotifyUpdate(const std::string& file_path, minidfs::FileUpdateType type) override {
            std::lock_guard<std::mutex> lock(mu_);  
            
            minidfs::FileInfo file_info;
            file_info.set_file_path(file_path);
            file_info.set_is_dir(fs::is_directory(file_path));
            file_info.set_hash(FileManager::GetFileHash(file_path));

            minidfs::FileUpdate res;
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
            std::lock_guard<std::mutex> lock(mu_);
            queue_.pop();
            if (!queue_.empty()) {
                StartWrite(&queue_.front());
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
        std::queue<minidfs::FileUpdate> queue_;
    };

    auto* reactor = new Reactor(this, request->client_id());

    this->pubsub_manager_->Subscribe(request->client_id(), reactor);

    
    return reactor;
}