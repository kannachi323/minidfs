#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <chrono>
#include <thread>
#include <atomic>
#include <unordered_map>
#include <filesystem>
#include "core/ui_registry.h"
#include "core/http_client.h"
#include "core/util.h"
#include <nlohmann/json.hpp>

namespace fs = std::filesystem;

namespace minidfs::panel {

    struct DeviceInfo {
        std::string name;
        std::string id;
        std::string hostname;  // peer_hostname for pinging
        std::string ip_address;
        bool is_online = false;
        std::string last_seen;
        std::string status_detail;
        uint64_t used_bytes = 0;
        uint64_t total_bytes = 0;
        std::string mount_path;
        std::string workspace_id;
    };

    struct DevicesState : public core::UIState {
        std::mutex mu;

        // Devices list
        std::vector<DeviceInfo> devices;

        // UI state
        std::string error_msg = "";
        std::string success_msg = "";

        // Filter/search
        char search_buffer[256] = "";
        bool filter_online_only = false;

        // Workspace filter
        std::string current_workspace_id = "";

        // Add Device modal
        bool show_add_device_modal = false;

        // Edit Device modal
        bool show_edit_device_modal = false;
        std::string editing_device_id = "";
        char editing_device_name[256] = "";
        char editing_mount_path[512] = "";

        // Delete confirmation
        bool show_delete_confirm = false;
        std::string deleting_device_id = "";
        std::string deleting_device_name = "";

        // Fetch state
        bool is_fetching_devices = false;
        std::chrono::steady_clock::time_point last_fetch_time = std::chrono::steady_clock::time_point{};
        static constexpr int fetch_interval_ms = 5000; // Fetch device list every 5 seconds

        // Ping state for online status checking
        std::atomic<bool> ping_thread_running{false};
        std::thread ping_thread;
        std::chrono::steady_clock::time_point last_ping_time = std::chrono::steady_clock::time_point{};
        static constexpr int ping_interval_ms = 300000; // Ping every 5 minutes (300000ms)

        ~DevicesState() {
            stop_ping_thread();
        }

        void start_ping_thread() {
            if (ping_thread_running.load()) {
                return;
            }
            ping_thread_running.store(true);
            ping_thread = std::thread([this]() {
                while (ping_thread_running.load()) {
                    ping_all_devices();
                    // Sleep in small increments to allow quick shutdown
                    for (int i = 0; i < 300 && ping_thread_running.load(); ++i) {
                        std::this_thread::sleep_for(std::chrono::seconds(1));
                    }
                }
            });
        }

        void stop_ping_thread() {
            ping_thread_running.store(false);
            if (ping_thread.joinable()) {
                ping_thread.join();
            }
        }

        // Ping a single device and return true if online
        bool ping_device(const std::string& hostname) {
            if (hostname.empty()) {
                return false;
            }
            std::string url = "http://localhost:3000/api/ts-ping?hostname=" + hostname;
            core::HttpResponse response = core::HttpClient::get().get(url);
            return response.status_code >= 200 && response.status_code < 300;
        }

        // Ping all devices and update their online status
        void ping_all_devices() {
            std::vector<std::pair<std::string, std::string>> devices_to_ping;

            // Collect devices to ping (minimize lock time)
            {
                std::lock_guard<std::mutex> lock(mu);
                for (const auto& device : devices) {
                    if (!device.hostname.empty()) {
                        devices_to_ping.push_back({device.id, device.hostname});
                    }
                }
            }

            // Ping each device
            for (const auto& [id, hostname] : devices_to_ping) {
                bool is_online = ping_device(hostname);

                // Update device status
                std::lock_guard<std::mutex> lock(mu);
                for (auto& device : devices) {
                    if (device.id == id) {
                        device.is_online = is_online;
                        break;
                    }
                }
            }

            last_ping_time = std::chrono::steady_clock::now();
        }

        void fetch_devices() {
            if (is_fetching_devices) {
                return;
            }

            auto now = std::chrono::steady_clock::now();
            if (last_fetch_time != std::chrono::steady_clock::time_point{} &&
                std::chrono::duration_cast<std::chrono::milliseconds>(now - last_fetch_time).count() < fetch_interval_ms) {
                return;
            }

            is_fetching_devices = true;
            error_msg = "";

            // Build URL with optional workspace_id filter
            std::string url = "http://localhost:3000/api/devices";
            if (!current_workspace_id.empty()) {
                url += "?workspace_id=" + current_workspace_id;
            }

            core::HttpResponse response = core::HttpClient::get().get(url);
            if (response.status_code >= 200 && response.status_code < 300) {
                try {
                    auto json = nlohmann::json::parse(response.body);

                    // Build a map of existing device online statuses to preserve them
                    std::unordered_map<std::string, bool> existing_status;
                    {
                        std::lock_guard<std::mutex> lock(mu);
                        for (const auto& device : devices) {
                            existing_status[device.id] = device.is_online;
                        }
                    }

                    std::vector<DeviceInfo> new_devices;

                    if (json.is_array()) {
                        for (const auto& device_json : json) {
                            DeviceInfo device;
                            device.id = device_json.value("id", "");
                            device.hostname = device_json.value("peer_hostname", "");
                            device.name = device_json.value("device_name", "");
                            // If device_name is empty, use peer_hostname as fallback
                            if (device.name.empty()) {
                                device.name = device.hostname;
                            }
                            device.ip_address = device_json.value("peer_address", "");
                            device.status_detail = device_json.value("peer_type", "");
                            device.mount_path = device_json.value("mount_path", "");
                            device.workspace_id = device_json.value("workspace_id", "");

                            // Create device mount_path directory if it doesn't exist
                            if (!device.mount_path.empty()) {
                                std::error_code ec;
                                fs::create_directories(device.mount_path, ec);
                            }

                            // Preserve existing online status if we have it
                            auto it = existing_status.find(device.id);
                            if (it != existing_status.end()) {
                                device.is_online = it->second;
                            } else {
                                device.is_online = false; // New device, default to offline until pinged
                            }

                            // Parse last_seen timestamp if available (for display only)
                            if (device_json.contains("last_seen")) {
                                device.last_seen = device_json["last_seen"].get<std::string>();
                            }

                            new_devices.push_back(device);
                        }
                    }

                    {
                        std::lock_guard<std::mutex> lock(mu);
                        devices = std::move(new_devices);
                    }

                    // Start ping thread if not already running
                    start_ping_thread();

                    // Trigger initial ping if this is first fetch or ping hasn't run yet
                    if (last_ping_time == std::chrono::steady_clock::time_point{}) {
                        // Run initial ping in a detached thread to not block UI
                        std::thread([this]() {
                            ping_all_devices();
                        }).detach();
                    }

                } catch (const std::exception& ex) {
                    error_msg = std::string("Failed to parse devices JSON: ") + ex.what();
                }
            } else {
                error_msg = "Failed to fetch devices (" + std::to_string(response.status_code) + ")";
            }

            last_fetch_time = now;
            is_fetching_devices = false;
        }
    };

}
