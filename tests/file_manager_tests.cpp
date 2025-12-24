#include <gtest/gtest.h>
#include <thread>
#include <atomic>
#include <vector>
#include "dfs/file_manager.h"

std::string mount_path = "storage/server";

TEST(FileManagerTest, AcquireWriteLock) {
    FileManager fm(mount_path);

    EXPECT_TRUE(fm.AcquireWriteLock("client1", "a.txt"));
    EXPECT_FALSE(fm.AcquireWriteLock("client2", "a.txt"));
}

TEST(FileManagerTest, WriteWithoutLockFails) {
    FileManager fm(mount_path);

    EXPECT_FALSE(fm.WriteFile("client1", "a.txt", "hello"));
}

TEST(FileManagerTest, WriteWithLockSucceeds) {
    FileManager fm(mount_path);

    EXPECT_TRUE(fm.AcquireWriteLock("client1", "a.txt"));
    EXPECT_TRUE(fm.WriteFile("client1", "a.txt", "hello"));
}

TEST(FileManagerTest, LockReleaseAllowsAnotherClient) {
    FileManager fm("/tmp/testfs");
    std::string client1 = "client_1";
    std::string client2 = "client_2";

    EXPECT_TRUE(fm.AcquireWriteLock(client1, "file.txt"));
    EXPECT_FALSE(fm.AcquireWriteLock(client2, "file.txt")); // should fail

    EXPECT_TRUE(fm.ReleaseWriteLock(client1, "file.txt"));
    EXPECT_TRUE(fm.AcquireWriteLock(client2, "file.txt")); // now should succeed
}


TEST(FileManagerTest, ConcurrentWrites) {
    FileManager fm(mount_path);

    const int num_threads = 8;
    std::atomic<int> success_count{0};
    std::string client_id_base = "client_";

    auto write_task = [&](int i) {
        std::string client_id = client_id_base + std::to_string(i);
        if (fm.AcquireWriteLock(client_id, "file.txt")) {  // Only proceed if lock acquired
            if (fm.WriteFile(client_id, "file.txt", "hello")) {
                success_count++;
            }
        }
    };

    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(write_task, i);
    }

    for (auto& t : threads) t.join();

    EXPECT_EQ(success_count, 1);  // Only 1 thread should actually write
}

TEST(FileManagerTest, ReadAfterWrite) {
    FileManager fm("/tmp/testfs");
    std::string client = "client_1";
    fm.AcquireWriteLock(client, "file.txt");

    EXPECT_TRUE(fm.WriteFile(client, "file.txt", "hello world"));

    minidfs::FileBuffer buf;
    EXPECT_TRUE(fm.ReadFile("file.txt", 0, 11, &buf));
    EXPECT_EQ(buf.data(), "hello world");
}

TEST(FileManagerTest, SameFileHasEqualHash) {
    FileManager fm(mount_path);
    std::string client = "client_1";
    fm.AcquireWriteLock(client, "file.txt");

    EXPECT_TRUE(fm.WriteFile(client, "file.txt", "hello world"));

    std::string hash1 = fm.GetFileHash(mount_path, "file.txt");
    std::string hash2 = fm.GetFileHash(mount_path, "file.txt");

    EXPECT_EQ(hash1, hash2);
}

TEST(FileManagerTest, DifferentFileHasDifferentHash) {
    FileManager fm(mount_path);
    std::string client = "client_1";
    fm.AcquireWriteLock(client, "file1.txt");
    fm.AcquireWriteLock(client, "file2.txt");

    EXPECT_TRUE(fm.WriteFile(client, "file1.txt", "hello world"));
    EXPECT_TRUE(fm.WriteFile(client, "file2.txt", "goodbye world"));

    std::string hash1 = fm.GetFileHash(mount_path, "file1.txt");
    std::string hash2 = fm.GetFileHash(mount_path, "file2.txt");

    EXPECT_NE(hash1, hash2);
}
 



