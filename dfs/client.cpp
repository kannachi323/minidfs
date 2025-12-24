#include <grpcpp/grpcpp.h>
#include <iostream>
#include "minidfs_client.h"
#include "utils.h"

int main() {
    auto channel = grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials());
    MiniDFSClient client(channel, "storage");

    std::string client_id = GetLocalIP();

    client.BeginSync(client_id);
    
    return 0;
}