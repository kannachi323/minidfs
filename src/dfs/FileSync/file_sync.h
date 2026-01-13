#pragma once


#include <memory>
#include "dfs/minidfs_client.h"
#include <mutex>

#define MAX_SYNC_BUFFER_SIZE 2 * 1024 * 1024

namespace minidfs {
    class FileSync {
    public:
        FileSync(std::shared_ptr<MiniDFSClient> client);
        virtual ~FileSync() = default;

        virtual void init_sync_resources() = 0;
        void start_sync();

    protected:
        virtual void sync_loop() = 0;
        virtual void process_changes() = 0;
        virtual void process_overflow() = 0;
    
    protected:
        std::shared_ptr<MiniDFSClient> client_;
        std::thread sync_thread_;
        bool running_;
    };
}
