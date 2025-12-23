#include <iostream>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>
#include "minidfs_impl.h" // Ensure this includes your MiniDFSImpl declaration

void RunServer(const std::string& mount_path) {
    std::string server_address("0.0.0.0:50051");
    MiniDFSImpl service(mount_path);

    grpc::ServerBuilder builder;
    // Listen on the given address without any authentication mechanism.
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    // Register "service" as the instance through which we'll communicate with clients.
    builder.RegisterService(&service);

    // Finally assemble the server.
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    std::cout << "MiniDFS Server listening on " << server_address << std::endl;
    std::cout << "Mount path: " << mount_path << std::endl;

    // Wait for the server to shutdown. 
    server->Wait();
}

int main(int argc, char** argv) {
    std::string mount_path = "./storage"; // Default storage path
    if (argc > 1) {
        mount_path = argv[1];
    }

    RunServer(mount_path);
    return 0;
}