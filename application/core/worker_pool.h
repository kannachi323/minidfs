#pragma once
#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>

struct Worker {
    std::function<void()> on_task;
    std::function<void()> on_finish;
    std::function<void()> on_error;
};


namespace minidfs {
    class WorkerPool {
    public:
        explicit WorkerPool(size_t thread_count = std::thread::hardware_concurrency());
        ~WorkerPool();

        void add(std::function<void()> on_task, std::function<void()> on_finish, std::function<void()> on_error);
    private:
        void worker_thread();

        std::vector<std::thread> workers_;
        std::queue<Worker> task_queue;
        std::mutex mu_;
        std::condition_variable cv_;
        bool stop_ = false;
    };    
};