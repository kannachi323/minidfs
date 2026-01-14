#ifdef __APPLE__

#include "file_sync_mac.h"
#include <vector>
#include <string>


namespace minidfs {
    FileSyncMac::FileSyncMac(std::shared_ptr<MiniDFSClient> client)
        : FileSync(std::move(client)), stream_(nullptr) {
    }

    FileSyncMac::~FileSyncMac() {
        if (stream_) {
            FSEventStreamStop(stream_);
            FSEventStreamInvalidate(stream_);
            FSEventStreamRelease(stream_);
        }
    }

    void FileSyncMac::file_sync_callback(ConstFSEventStreamRef streamRef,
        void *clientCallBackInfo,
        size_t numEvents,
        void *eventPaths,
        const FSEventStreamEventFlags eventFlags[],
        const FSEventStreamEventId eventIds[]) 
    {
        FileSyncMac* sync = static_cast<FileSyncMac*>(clientCallBackInfo);
        sync->handle_events(numEvents, (char **)eventPaths, eventFlags);
    }


    void FileSyncMac::init_sync_resources() {
        
        std::cout << "Starting file watcher on path: " << client_->GetClientMountPath() << std::endl;
        CFStringRef cfPath = CFStringCreateWithCString(NULL, client_->GetClientMountPath().c_str(), kCFStringEncodingUTF8);
        CFArrayRef pathsToWatch = CFArrayCreate(NULL, (const void **)&cfPath, 1, NULL);

        FSEventStreamContext context = {0, (void*)this, NULL, NULL, NULL};

        stream_ = FSEventStreamCreate(
            NULL, &FileSyncMac::file_sync_callback, &context, pathsToWatch,
            kFSEventStreamEventIdSinceNow, 2, kFSEventStreamCreateFlagFileEvents
        );
        dispatch_queue_t dq = dispatch_queue_create("com.minidfs.watcher", DISPATCH_QUEUE_SERIAL);
        FSEventStreamSetDispatchQueue(stream_, dq);
        CFRelease(pathsToWatch);
        CFRelease(cfPath);
    }

    void FileSyncMac::start_sync() {
        FSEventStreamStart(stream_);
    }
    
    void FileSyncMac::handle_events(size_t numEvents, char **eventPaths, const FSEventStreamEventFlags eventFlags[]) {
        for (size_t i = 0; i < numEvents; ++i) {
            std::string path(eventPaths[i]);
            bool is_dir = (eventFlags[i] & kFSEventStreamEventFlagItemIsDir) != 0;

            if (eventFlags[i] & kFSEventStreamEventFlagItemCreated) {
                on_file_created(path, is_dir);
            } else if (eventFlags[i] & kFSEventStreamEventFlagItemRemoved) {
                on_file_removed(path, is_dir);
            } else if (eventFlags[i] & kFSEventStreamEventFlagItemModified) {
                on_file_modified(path, is_dir);
            } else if (eventFlags[i] & kFSEventStreamEventFlagItemRenamed) {
                on_file_renamed(path, is_dir);
            } else if (eventFlags[i] & kFSEventStreamEventFlagMustScanSubDirs) {
                std::cout << "Overflow detected, must scan subdirectories." << std::endl;
            }
        }
    }
    
}

#endif