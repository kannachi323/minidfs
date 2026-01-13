#include <gtest/gtest.h>
#include <thread>
#include <atomic>
#include <vector>
#include <string>
#include <fstream>
#include <filesystem>
#include <random>
#include "dfs/minidfs_client.h"
#include "dfs/minidfs_impl.h"

namespace fs = std::filesystem;

class MiniDFSSingleClientTest : public ::testing::Test {
protected:
    const std::string server_mount = "server";
    const std::string client_mount = "client";

    std::unique_ptr<MiniDFSImpl> server_impl;
    std::unique_ptr<grpc::Server> server;
    std::shared_ptr<grpc::Channel> shared_channel;
    std::unique_ptr<MiniDFSClient> client;

    void CreateLocalFile(const std::string& file_path, const std::string& content) {
        fs::path p(file_path);
        if (p.has_parent_path()) {
            fs::create_directories(p.parent_path());
        }
        std::ofstream ofs(file_path, std::ios::binary | std::ios::trunc);
        ofs.write(content.data(), content.size());
        ofs.flush();
        ofs.close();
    }

    std::string ReadLocalFile(const std::string& file_path) {
        std::ifstream ifs(file_path, std::ios::binary | std::ios::ate);
        if (!ifs) return "";

        std::streamsize size = ifs.tellg();
        ifs.seekg(0, std::ios::beg);

        std::string buffer(size, '\0');
        if (!ifs.read(buffer.data(), size)) {
            return "";
        }
        
        return buffer;
    }

    void SetUp() override {
        fs::create_directories(server_mount);
        fs::create_directories(client_mount);

        // Server Setup
        server_impl = std::make_unique<MiniDFSImpl>(server_mount);
        grpc::ServerBuilder builder;
        builder.AddListeningPort("localhost:50051", grpc::InsecureServerCredentials());
        builder.RegisterService(server_impl.get());
        server = builder.BuildAndStart();

        // Client Setup
        shared_channel = grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials());
        client = std::make_unique<MiniDFSClient>(shared_channel, client_mount);
    }

    void TearDown() override {
        server_impl->file_manager_->ReleaseAllLocks();
        server->Shutdown();
        fs::remove_all(server_mount);
        fs::remove_all(client_mount);
    }
};

TEST_F(MiniDFSSingleClientTest, ListFilesEmptyDirectory) {
    fs::path dir_path = fs::path(server_mount) / fs::path(client_mount) / "list_test";
    fs::path dir_path_client = fs::path(client_mount) / "list_test";
    fs::create_directories(dir_path);

    minidfs::ListFilesRes response;
    grpc::StatusCode status = client->ListFiles(dir_path_client.string(), &response);
    ASSERT_EQ(status, grpc::StatusCode::OK);
    EXPECT_EQ(response.files_size(), 0);
}

TEST_F(MiniDFSSingleClientTest, ListFilesWithFiles) {
    fs::path dir_path = fs::path(server_mount) / fs::path(client_mount) / "list_test";
    fs::path dir_path_client = fs::path(client_mount) / "list_test";
    fs::create_directories(dir_path);
    CreateLocalFile((dir_path / "file1.txt").string(), "Content 1");
    CreateLocalFile((dir_path / "file2.txt").string(), "Content 2");

    minidfs::ListFilesRes response;
    grpc::StatusCode status = client->ListFiles(dir_path_client.string(), &response);
    ASSERT_EQ(status, grpc::StatusCode::OK);
    EXPECT_EQ(response.files_size(), 2);

    std::vector<std::string> file_names;
    for (int i = 0; i < response.files_size(); ++i) {
        file_names.push_back(fs::path(response.files(i).file_path()).filename().string());
    }
    EXPECT_NE(std::find(file_names.begin(), file_names.end(), "file1.txt"), file_names.end());
    EXPECT_NE(std::find(file_names.begin(), file_names.end(), "file2.txt"), file_names.end());
}

TEST_F(MiniDFSSingleClientTest, ListFilesNonExistentDirectory) {
    fs::path dir_path_client = fs::path(client_mount) / "non_existent_dir";

    minidfs::ListFilesRes response;
    grpc::StatusCode status = client->ListFiles(dir_path_client.string(), &response);
    ASSERT_EQ(status, grpc::StatusCode::NOT_FOUND);
}

TEST_F(MiniDFSSingleClientTest, ListFilesNonValidDirectory) {
    fs::path file_path = fs::path(server_mount) / fs::path(client_mount) / "not_a_dir.txt";
    fs::path file_path_client = fs::path(client_mount) / "not_a_dir.txt";
    CreateLocalFile(file_path.string(), "I am a file, not a directory.");

    minidfs::ListFilesRes response;
    grpc::StatusCode status = client->ListFiles(file_path_client.string(), &response);
    ASSERT_EQ(status, grpc::StatusCode::FAILED_PRECONDITION);
}

TEST_F(MiniDFSSingleClientTest, ListFilesWithOtherDirectories) {
    fs::path dir_path = fs::path(server_mount) / fs::path(client_mount) / "list_test";
    fs::path dir_path_client = fs::path(client_mount) / "list_test";
    fs::create_directories(dir_path);
    CreateLocalFile((dir_path / "file1.txt").string(), "Content 1");
    fs::create_directories(dir_path / "subdir");

    minidfs::ListFilesRes response;
    grpc::StatusCode status = client->ListFiles(dir_path_client.string(), &response);
    ASSERT_EQ(status, grpc::StatusCode::OK);
    EXPECT_EQ(response.files_size(), 2);

    std::vector<std::string> file_names;
    for (int i = 0; i < response.files_size(); ++i) {
        file_names.push_back(fs::path(response.files(i).file_path()).filename().string());
        if (file_names[i] == "subdir") {
            EXPECT_TRUE(response.files(i).is_dir());
        }
    }
    EXPECT_NE(std::find(file_names.begin(), file_names.end(), "file1.txt"), file_names.end());
}

TEST_F(MiniDFSSingleClientTest, StoreNonExistentFile) {
    const std::string client_id = "client1";
    const std::string client_file_path = client_mount + "/does_not_exist.txt";

    grpc::StatusCode status = client->StoreFile(client_id, client_file_path);
    EXPECT_EQ(status, grpc::StatusCode::NOT_FOUND);
}

TEST_F(MiniDFSSingleClientTest, StoreSmallFile) {
    const std::string client_id = "client1";
    fs::path client_file_path = fs::path(client_mount) / "test.txt";
    const std::string content = "Hello, MiniDFS!";

    CreateLocalFile(client_file_path.string(), content);

    grpc::StatusCode status = client->StoreFile(client_id, client_file_path.string());
    ASSERT_EQ(status, grpc::StatusCode::OK);

    fs::path server_file_path = fs::path(server_mount) / fs::path(client_mount) / "test.txt";
    ASSERT_TRUE(fs::exists(server_file_path));

    EXPECT_EQ(
        FileManager::GetFileHash(client_file_path.string()), 
        FileManager::GetFileHash(server_file_path.string()));
}

TEST_F(MiniDFSSingleClientTest, StoreLargeFile) {
    const std::string client_id = "client1";
    fs::path client_file_path = fs::path(client_mount) / "large_1gb.bin";
    

    const size_t size = 100 * 1024 * 1024;
    std::ofstream ofs(client_file_path, std::ios::binary | std::ios::out);
    ASSERT_TRUE(ofs.is_open());

    ofs.seekp(size - 1);
    ofs.write("\0", 1);
    ofs.close();

    ASSERT_EQ(fs::file_size(client_file_path), size);

    grpc::StatusCode status = client->StoreFile(client_id, client_file_path.string());
    ASSERT_EQ(status, grpc::StatusCode::OK);

    fs::path server_file_path = fs::path(server_mount) / fs::path(client_mount) / "large_1gb.bin";
    ASSERT_TRUE(fs::exists(server_file_path)) << "File missing on server: " << server_file_path;
    ASSERT_EQ(fs::file_size(server_file_path), size);

    EXPECT_EQ(
        FileManager::GetFileHash(client_file_path.string()), 
        FileManager::GetFileHash(server_file_path.string())
    );
}

TEST_F(MiniDFSSingleClientTest, FetchSmallFile) {
    const std::string client_id = "client1";
    fs::path server_file_path = fs::path(server_mount) / fs::path(client_mount) / "fetch_test.txt";
    const std::string content = "Fetching this file from server.";

    CreateLocalFile(server_file_path.string(), content);

    fs::path client_file_path = fs::path(client_mount) / "fetch_test.txt";
    grpc::StatusCode status = client->FetchFile(client_id, client_file_path.string());
    ASSERT_EQ(status, grpc::StatusCode::OK);
    ASSERT_TRUE(fs::exists(client_file_path));

    EXPECT_EQ(
        FileManager::GetFileHash(server_file_path.string()), 
        FileManager::GetFileHash(client_file_path.string())
    );
}

TEST_F(MiniDFSSingleClientTest, FetchLargeFile) {
    const std::string client_id = "client1";
    fs::path server_file_path = fs::path(server_mount) / fs::path(client_mount) / "large_fetch_1gb.bin";

    fs::create_directories(server_file_path.parent_path());

    const size_t size = 100 * 1024 * 1024;
    std::ofstream ofs(server_file_path, std::ios::binary | std::ios::out);
    ASSERT_TRUE(ofs.is_open());

    ofs.seekp(size - 1);
    ofs.write("\0", 1);
    ofs.close();

    ASSERT_EQ(fs::file_size(server_file_path), size);

    fs::path client_file_path = fs::path(client_mount) / "large_fetch_1gb.bin";
    grpc::StatusCode status = client->FetchFile(client_id, client_file_path.string());
    ASSERT_EQ(status, grpc::StatusCode::OK);
    ASSERT_TRUE(fs::exists(client_file_path));
    ASSERT_EQ(fs::file_size(client_file_path), size);

    EXPECT_EQ(
        FileManager::GetFileHash(server_file_path.string()), 
        FileManager::GetFileHash(client_file_path.string())
    );
}

TEST_F(MiniDFSSingleClientTest, RemoveFile) {
    const std::string client_id = "client1";
    fs::path client_file_path = fs::path(client_mount) / "delete_test.txt";
    const std::string content = "This file will be deleted from the server.";

    CreateLocalFile(client_file_path.string(), content);

    grpc::StatusCode store_status = client->StoreFile(client_id, client_file_path.string());
    ASSERT_EQ(store_status, grpc::StatusCode::OK);

    fs::path server_file_path = fs::path(server_mount) / fs::path(client_mount) / "delete_test.txt";
    ASSERT_TRUE(fs::exists(server_file_path));

    grpc::StatusCode delete_status = client->RemoveFile(client_id, client_file_path.string());
    ASSERT_EQ(delete_status, grpc::StatusCode::OK);

    EXPECT_FALSE(fs::exists(server_file_path));
}

TEST_F(MiniDFSSingleClientTest, DeleteNonExistentFile) {
    const std::string client_id = "client1";
    fs::path client_file_path = fs::path(client_mount) / "non_existent_delete.txt";

    grpc::StatusCode delete_status = client->RemoveFile(client_id, client_file_path.string());
    ASSERT_EQ(delete_status, grpc::StatusCode::NOT_FOUND);
}