#include <gtest/gtest.h>
#include <thread>
#include <atomic>
#include <vector>
#include <string>
#include <chrono>
#include <fstream>
#include "dfs/minidfs_client.h"

class MiniDFSClientTest : public ::testing::Test {
protected:
    static std::atomic<int> file_counter;
    std::shared_ptr<grpc::Channel> shared_channel; // Define here for thread access
    std::unique_ptr<MiniDFSClient> client;

    void SetUp() override {
        std::string server_address = "localhost:50051";
        shared_channel = grpc::CreateChannel(server_address, grpc::InsecureChannelCredentials());
        client = std::make_unique<MiniDFSClient>(shared_channel, "storage");
    }

    // Returns a unique filename for this specific call
    std::string GetTestFile() {
        return "test_file_" + std::to_string(file_counter.fetch_add(1)) + ".txt";
    }
};

std::atomic<int> MiniDFSClientTest::file_counter{0};

TEST_F(MiniDFSClientTest, AcquireWriteLock) {
    std::string filename = GetTestFile(); // Store it to use the SAME name twice
    
    grpc::StatusCode status1 = client->GetWriteLock("client1", filename);
    EXPECT_EQ(status1, grpc::StatusCode::OK);

    // Now this correctly attempts to lock the same file
    grpc::StatusCode status2 = client->GetWriteLock("client2", filename);
    EXPECT_NE(status2, grpc::StatusCode::OK); 
}

TEST_F(MiniDFSClientTest, WriteWithLockSucceeds) {
    // StoreFile internally calls GetTestFile() essentially
    EXPECT_EQ(client->StoreFile("client1", GetTestFile(), "hello world"), grpc::StatusCode::OK);
}

TEST_F(MiniDFSClientTest, WriteFailsIfLockedByOther) {
    std::string filename = GetTestFile();
    std::string content = "should fail";

    // 1. "client_blocker" grabs the lock for our specific test file
    EXPECT_EQ(client->GetWriteLock("client_blocker", filename), grpc::StatusCode::OK);

    // 2. "client_victim" tries to StoreFile to the SAME filename
    grpc::StatusCode status = client->StoreFile("client_victim", filename, content);
    EXPECT_NE(status, grpc::StatusCode::OK);
}

TEST_F(MiniDFSClientTest, ConcurrentWrites) {
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

TEST_F(MiniDFSClientTest, ReadAfterWrite) {
    std::string filename = GetTestFile();
    std::string content = "Data Integrity Check";
    std::string client_id = "client_reader";

    // 1. Write the file
    ASSERT_EQ(client->StoreFile(client_id, filename, "asdfasdf"), grpc::StatusCode::OK);

}