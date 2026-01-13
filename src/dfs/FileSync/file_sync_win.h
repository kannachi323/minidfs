#pragma once

#ifdef _WIN32

#include <windows.h>
#include <winbase.h>
#include <iostream>
#include "file_sync.h"

namespace minidfs {
    class FileSyncWin32 : public FileSync {
    public:
        FileSyncWin32(std::shared_ptr<MiniDFSClient> client);
        ~FileSyncWin32();

        void init_sync_resources() override;
    
    private:
        void sync_loop() override;
        void process_changes() override;
        void process_overflow() override;
        void handle_action(DWORD action, const std::wstring& file_name);
    private:
        HANDLE directory_handle_;
        LPVOID buffer_[MAX_SYNC_BUFFER_SIZE];
        OVERLAPPED overlapped_;
        HANDLE stop_signal_;
        DWORD bytes_transferred_;
    };
}
#endif

