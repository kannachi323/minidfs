#pragma once

#ifdef __APPLE__
#include <CoreServices/CoreServices.h>
#include <iostream>
#include "file_sync.h"

namespace minidfs {
    class FileSyncMac : public FileSync {
    public:
        FileSyncMac(std::shared_ptr<MiniDFSClient> client);
        ~FileSyncMac();

        void init_sync_resources() override;
        void start_sync() override;
    
    private:
        static void file_sync_callback(ConstFSEventStreamRef streamRef,
            void *clientCallBackInfo,
            size_t numEvents,
            void *eventPaths,
            const FSEventStreamEventFlags eventFlags[],
            const FSEventStreamEventId eventIds[]);

        void handle_events(size_t numEvents, char **eventPaths, const FSEventStreamEventFlags eventFlags[]);
    private:
        FSEventStreamRef stream_;
        
    };
}
#endif

