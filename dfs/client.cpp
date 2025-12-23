#include "dfs/minidfs_client.h"
#include <grpcpp/grpcpp.h>
#include <iostream>

int main() {
    auto channel = grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials());
    MiniDFSClient client(channel, "storage");

}