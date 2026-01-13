#pragma once

#include <windows.h>
#include <winbase.h>
#include <memory>
#include "minidfs_client.h"
#include <mutex>

#define MAX_SYNC_BUFFER_SIZE 2 * 1024 * 1024

namespace minidfs {
    class FileSyncEngine {
    public:
        FileSyncEngine(std::shared_ptr<MiniDFSClient> client);
        ~FileSyncEngine();

        void init_sync_resources();
        void start_sync();

    private:
        void sync_loop();
        void process_changes(DWORD bytes_returned);
        void process_overflow();
        void handle_action(DWORD action, const std::wstring& file_name);


    private:
        std::shared_ptr<MiniDFSClient> client_;
        std::thread sync_thread_;
        HANDLE directory_handle_;
        LPVOID buffer_[MAX_SYNC_BUFFER_SIZE];
        OVERLAPPED overlapped_;
        HANDLE stop_signal_;
        bool running_;
    };
}
