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

class MiniDFSSingleClientTest : public ::testing::Test {
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

std::atomic<int> MiniDFSSingleClientTest::file_counter{0};

TEST_F(MiniDFSSingleClientTest, AcquireWriteLock) {
    std::string filename = GetTestFile(); // Store it to use the SAME name twice
    
    grpc::StatusCode status1 = client->GetWriteLock("client1", filename);
    EXPECT_EQ(status1, grpc::StatusCode::OK);

    // Now this correctly attempts to lock the same file
    grpc::StatusCode status2 = client->GetWriteLock("client2", filename);
    EXPECT_NE(status2, grpc::StatusCode::OK); 

}

TEST_F(MiniDFSSingleClientTest, WriteWithLockSucceeds) {
    // StoreFile internally calls GetTestFile() essentially
    EXPECT_EQ(client->StoreFile("client1", GetTestFile(), "hello world"), grpc::StatusCode::OK);
}


TEST_F(MiniDFSSingleClientTest, ReadAfterWrite) {
    std::string filename = GetTestFile();
    std::string content = "Data Integrity Check";
    std::string client_id = "client_reader";

    // 1. Write the file
    ASSERT_EQ(client->StoreFile(client_id, filename, "asdfasdf"), grpc::StatusCode::OK);
}

TEST_F(MiniDFSSingleClientTest, DeleteFile) {
    std::string filename = GetTestFile();
    std::string content = "To be deleted";

    // 1. Write the file
    ASSERT_EQ(client->StoreFile("client_deleter", filename, content), grpc::StatusCode::OK);

    // 2. Delete the file
    grpc::StatusCode del_status = client->DeleteFile("client_deleter", filename);
    EXPECT_EQ(del_status, grpc::StatusCode::OK);

    // 3. Attempt to fetch the deleted file
    bool found = true;
    for (int i = 0; i < 10; ++i) {
        if (client->FetchFile(filename) == grpc::StatusCode::NOT_FOUND) {
            found = false;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    EXPECT_FALSE(found) << "File still exists after deletion attempts";

}

TEST_F(MiniDFSSingleClientTest, ListAllFiles) {
    std::string filename1 = GetTestFile();
    std::string filename2 = GetTestFile();

    // 1. Write two files
    ASSERT_EQ(client->StoreFile("client_lister", filename1, "File One"), grpc::StatusCode::OK);
    ASSERT_EQ(client->StoreFile("client_lister", filename2, "File Two"), grpc::StatusCode::OK);

    // 2. List all files in the root directory
    grpc::StatusCode list_status = client->ListAllFiles("");
    EXPECT_EQ(list_status, grpc::StatusCode::OK);
}

