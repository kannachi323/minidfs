#include <gtest/gtest.h>
#include <thread>
#include <atomic>
#include <vector>
#include "dfs/file_manager.h"

TEST(FileManagerTest, AcquireWriteLock) {
    FileManager fm("storage");

    EXPECT_TRUE(fm.AcquireWriteLock("client1", "a.txt"));
    EXPECT_FALSE(fm.AcquireWriteLock("client2", "a.txt"));
}

TEST(FileManagerTest, WriteWithoutLockFails) {
    FileManager fm("storage");

    EXPECT_FALSE(fm.WriteFile("client1", "a.txt", "hello"));
}

TEST(FileManagerTest, WriteWithLockSucceeds) {
    FileManager fm("storage");

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
    FileManager fm("/tmp/testfs");

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
 



