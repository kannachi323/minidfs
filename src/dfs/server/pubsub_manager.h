#pragma once

#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include "proto_src/minidfs.grpc.pb.h"

class IPubSubReactor {
public:
    virtual ~IPubSubReactor() = default;
    virtual void NotifyUpdate(const std::string& file_path, minidfs::FileUpdateType type) = 0;
};

class PubSubManager {
public:
    bool Subscribe(const std::string& client_id, IPubSubReactor* reactor);
    bool Unsubscribe(const std::string& client_id, IPubSubReactor* reactor);
    void Publish(const std::string& client_id, const std::string& file_path, minidfs::FileUpdateType type);

private:
    std::mutex mu_;
    std::unordered_map<std::string, IPubSubReactor*> registry_;
};