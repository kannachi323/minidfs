#include "worker_pool.h"
#include <iostream>

namespace minidfs {
    WorkerPool::WorkerPool(size_t thread_count) {
        std::cout << "[DEBUG] Initializing WorkerPool with " << thread_count << " threads." << std::endl;
        for (size_t i = 0; i < thread_count; ++i) {
            workers_.emplace_back(&WorkerPool::worker_thread, this);
        }
    }

    WorkerPool::~WorkerPool() {
        {
            std::unique_lock<std::mutex> lock(mu_);
            stop_ = true;
        }
        cv_.notify_all();
        for (std::thread& worker : workers_) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }

    void WorkerPool::add(std::function<void()> on_task, std::function<void()> on_finish, std::function<void()> on_error) {
        {
            std::unique_lock<std::mutex> lock(mu_);
            task_queue.push(Worker{on_task, on_finish, on_error});
        }
        cv_.notify_one();
    }

    void WorkerPool::worker_thread() {
        while (true) {
            Worker worker;
            {
                std::unique_lock<std::mutex> lock(mu_);
                cv_.wait(lock, [this]() { return stop_ || !task_queue.empty(); });
                if (stop_ && task_queue.empty()) {
                    return;
                }
                worker = std::move(task_queue.front());
                task_queue.pop();
            }
            try {
                if (worker.on_task) {
                    worker.on_task();
                }
                if (worker.on_finish) {
                    worker.on_finish();
                }
            } catch (...) {
                if (worker.on_error) {
                    worker.on_error();
                }
            }
        }
    }
}