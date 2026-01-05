#include <iostream>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>
#include <filesystem>
#include "dfs/minidfs_impl.h"

namespace fs = std::filesystem;

int main(int argc, char** argv) {
    std::string mount_path = "minidfs";
    if (argc > 1) {
        mount_path = argv[1];
    }
    fs::create_directory(mount_path);

    std::string server_address("0.0.0.0:50051");
    MiniDFSImpl service(mount_path);

    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());

    server->Wait();

    return 0;
}