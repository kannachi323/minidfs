#include "file_sync.h"

namespace minidfs {
    FileSync::FileSync(std::shared_ptr<MiniDFSClient> client)
        : client_(std::move(client)) {
        
    }
    void FileSync::start_sync() {
        running_ = true;
        sync_thread_ = std::thread(&FileSync::sync_loop, this);
    }
}
