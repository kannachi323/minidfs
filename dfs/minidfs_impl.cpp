#include "minidfs_impl.h"

#include <filesystem>
#include <chrono>

namespace fs = std::filesystem;

MiniDFSImpl::MiniDFSImpl(const std::string& mount_path) {
    file_manager_ = new FileManager(mount_path);
    mount_path_ = mount_path;
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
            fs::path full_path = service_->file_manager_->ResolvePath(service_->mount_path_, req->file_path());

            if (!fs::exists(full_path, ec)) {
                Finish(grpc::Status(grpc::StatusCode::NOT_FOUND, "File not found"));
                return;
            }

            response_->set_file_path(request_->file_path());
            response_->set_size(fs::file_size(full_path, ec));

            auto ftime = fs::last_write_time(full_path, ec);
            auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now()
            );
            response_->set_last_modified(
                std::chrono::duration_cast<std::chrono::seconds>(sctp.time_since_epoch()).count()
            );

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
            fs::path full_dir_path = service_->file_manager_->ResolvePath(service_->mount_path_, req->dir_path());
            std::cout << "Listing files in directory: " << full_dir_path << std::endl;
            for (auto& entry : fs::recursive_directory_iterator(full_dir_path)) {
                if (!entry.is_regular_file()) continue;

                auto file_info = res_->add_files();
                fs::path rel = fs::relative(entry.path(), full_dir_path);
                file_info->set_file_path(rel.string());
                file_info->set_size(entry.file_size(ec));

                auto ftime = entry.last_write_time(ec);
                auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                    ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now()
                );
                file_info->set_last_modified(
                    std::chrono::duration_cast<std::chrono::seconds>(sctp.time_since_epoch()).count()
                );
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
            fs::path full_path = service_->file_manager_->ResolvePath(service_->mount_path_, req->file_path());

            if (!fs::exists(full_path, ec)) {
                Finish(grpc::Status(grpc::StatusCode::NOT_FOUND, "File not found"));
                delete this;
                return;
            }

            fs::remove(full_path, ec);
            if (ec) {
                Finish(grpc::Status(grpc::StatusCode::INTERNAL, "Failed to delete file"));
                delete this;
                return;
            }

            res_->set_success(true);
            Finish(grpc::Status::OK);
            delete this;
        }

        void OnDone() override {}
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
                std::cout << current_.data() << std::endl;
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
                const minidfs::FetchFileReq* req)
            : service_(service), req_(req)
        {
            if (service_->file_manager_->FileExists(req_->file_path())) {
                Finish(grpc::Status(grpc::StatusCode::NOT_FOUND, "File not found"));
                return;
            }
            offset_ = 0;
            NextWrite();
        }

        void OnWriteDone(bool ok) override {
            if (!ok) {
                Finish(grpc::Status::CANCELLED);
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
        const size_t CHUNK_SIZE = 64 * 1024;
    };
    
    return new Reactor(this, request);
}
