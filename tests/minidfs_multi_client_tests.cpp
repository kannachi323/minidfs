#include <gtest/gtest.h>
#include <thread>
#include <atomic>
#include <vector>
#include <string>
#include <chrono>
#include <fstream>
#include "dfs/minidfs_client.h"
#include "dfs/minidfs_impl.h"
#include "dfs/utils.h"

class MiniDFSMultiClientTest : public ::testing::Test {
protected:
    static std::atomic<int> file_counter;
    
    // Server components
    std::unique_ptr<MiniDFSImpl> server_impl;
    std::unique_ptr<grpc::Server> server;
    
    // Client components
    std::shared_ptr<grpc::Channel> shared_channel;
    std::unique_ptr<MiniDFSClient> client;

    void SetUpClient() {
        shared_channel = grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials());
        client = std::make_unique<MiniDFSClient>(shared_channel, "storage/client");
        std::string client_id = GetLocalIP();
        client->BeginSync(client_id);
    }

    void SetUpServer() {
        std::string server_address = "localhost:50051";
        server_impl = std::make_unique<MiniDFSImpl>("storage/server");

        grpc::ServerBuilder builder;
        builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
        builder.RegisterService(server_impl.get());
        server = builder.BuildAndStart();
    }

    void SetUp() override {
        SetUpServer();
        SetUpClient();
    }

    void TearDown() override {
        client->EndSync();
        client.reset();
        server->Shutdown();
        // server_impl and client are cleaned up by unique_ptr
    }

    // This now works perfectly because we can access the server_impl instance directly
    void ResetState() {
        server_impl->file_manager_->mu_.lock();
        server_impl->file_manager_->write_locks_.clear();
    }

    std::string GetTestFile() {
        return "test_file_" + std::to_string(file_counter.fetch_add(1)) + ".txt";
    }
};

std::atomic<int> MiniDFSMultiClientTest::file_counter{0};



TEST_F(MiniDFSMultiClientTest, WriteFailsIfLockedByOther) {
    std::string filename = GetTestFile();
    std::string content = "should fail";

    // 1. "client_blocker" grabs the lock for our specific test file
    EXPECT_EQ(client->GetWriteLock("client_blocker", filename), grpc::StatusCode::OK);

    // 2. "client_victim" tries to StoreFile to the SAME filename
    grpc::StatusCode status = client->StoreFile("client_victim", filename, content);
    EXPECT_NE(status, grpc::StatusCode::OK);
}

TEST_F(MiniDFSMultiClientTest, ConcurrentWrites) {
    const int num_threads = 8;
    std::atomic<int> success_count{0};
    std::string filename = GetTestFile(); // All threads compete for this ONE file

    auto write_task = [&](int i) {
        MiniDFSClient thread_client(shared_channel, "storage");
        std::string client_id = "client_" + std::to_string(i);

        grpc::StatusCode status = thread_client.StoreFile(client_id, filename, "mine!");
        if (status == grpc::StatusCode::OK) {
            success_count++;
        }
    };

    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(write_task, i);
    }

    for (auto& t : threads) t.join();

    // Exactly 1 thread should succeed in acquiring the lock and writing
    EXPECT_EQ(success_count, 1);
}
