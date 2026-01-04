#pragma once

#include <string>
#include <functional>
#include <unordered_map>
#include <memory>


namespace minidfs {
    struct UIState {
        virtual ~UIState() = default;
    };

    class UIRegistry {
    public:
        template<typename T>
        T& get_state(const std::string& key) {
            if (states_.find(key) == states_.end()) {
                states_[key] = std::make_unique<T>();
            }
            return static_cast<T&>(*states_[key]);
        }

        template<typename T>
        void update_state(const std::string& key, std::function<void(T&)> callback) {
            T& state = get_state<T>(key);
            if (callback) {
                callback(state);
            }
        }
    private:
        std::unordered_map<std::string, std::unique_ptr<UIState>> states_;
    };
};
