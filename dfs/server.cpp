#include "server.h"

#include <filesystem>
#include <chrono>

namespace fs = std::filesystem;

grpc::ServerUnaryReactor* MiniDFSImpl::GetFileStatus(
    grpc::CallbackServerContext* context, 
    const minidfs::FileStatusReq* request, 
    minidfs::FileInfo* response
) {
    auto* reactor = context->DefaultReactor();
    std::error_code ec;
    
    fs::path full_path = fs::path(this->mount_path) / request->filename();

    if (!fs::exists(full_path, ec)) {
        reactor->Finish(grpc::Status(grpc::StatusCode::NOT_FOUND, "File not found"));
        return reactor;
    }

    uintmax_t size = fs::file_size(full_path, ec);
    if (ec) {
        reactor->Finish(grpc::Status(grpc::StatusCode::INTERNAL, "Failed to get file size: " + ec.message()));
        return reactor;
    }

    auto ftime = fs::last_write_time(full_path, ec);
    if (ec) {
        reactor->Finish(grpc::Status(grpc::StatusCode::INTERNAL, "Failed to get file time: " + ec.message()));
        return reactor;
    }

    // 3. Convert file_time_type to Unix Timestamp (Seconds)
    // In C++20, this becomes much easier with clock_cast, but for C++17:
    auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now()
    );
    long long seconds = std::chrono::duration_cast<std::chrono::seconds>(sctp.time_since_epoch()).count();

    // 4. Populate Proto
    response->set_filename(request->filename());
    response->set_size(size);
    response->set_modified_at(seconds);
    response->set_is_directory(fs::is_directory(full_path, ec));

    reactor->Finish(grpc::Status::OK);
    return reactor;
}