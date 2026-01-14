#include "pubsub_manager.h"

bool PubSubManager::Subscribe(const std::string& client_id, IPubSubReactor* reactor) {
    if (!reactor || registry_.contains(client_id)) return false;
    
    {
        std::lock_guard<std::mutex> lock(mu_);
        registry_[client_id] = reactor;
    }

    return true;
}

bool PubSubManager::Unsubscribe(const std::string& client_id, IPubSubReactor* reactor) {
    if (!reactor || !registry_.contains(client_id)) return false;
    
    {
        std::lock_guard<std::mutex> lock(mu_);
        registry_.erase(client_id);
    }
    
    return true;
}

void PubSubManager::Publish(const std::string& client_id, const std::string& file_path, minidfs::FileUpdateType type) {
    std::lock_guard<std::mutex> lock(mu_);

    
    for (const auto& entry : registry_) {
        const std::string& cid = entry.first;
        IPubSubReactor* reactor = entry.second;
        
        if (reactor && cid != client_id) {
            reactor->NotifyUpdate(file_path, type);
        } else {
            registry_.erase(client_id);
        }
    }
}