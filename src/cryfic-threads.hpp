#pragma once

#include <atomic>
#include <thread>
#include <vector>
#include <functional>
#include <cassert>
#include <cstdint>
#include <memory>
#include <immintrin.h>

#include "rigtorp/MPMCQueue.h"

#include "cryfic-opts.hpp"
#include "cryfic-db.hpp"

// Worker pool using MPMCQueue
class MPMCWorkerPool {
public:
    explicit MPMCWorkerPool(size_t num_threads, size_t queue_capacity)
        : queue_(queue_capacity)
        , result_(0)
        , running_(true)
        , pending_tasks_(0)
    {
        workers_.reserve(num_threads);
        for (size_t i = 0; i < num_threads; ++i) {
            workers_.emplace_back([this] { worker_loop(); });
        }
    }

    ~MPMCWorkerPool() {
        stop();
    }

    void submit(ProcessRowTask&& task) {
        pending_tasks_.fetch_add(1, std::memory_order_relaxed);
        queue_.push(std::move(task));
    }

    void wait() {
        // Spin-wait until all tasks are processed
        while (pending_tasks_.load(std::memory_order_acquire) > 0) {
            std::this_thread::yield();
        }
    }

    void stop() {
        if (running_.exchange(false)) {
            // Push empty tasks to wake up all workers
            for (size_t i = 0; i < workers_.size(); ++i) {
                queue_.push(ProcessRowTask{nullptr, 0, "", FileStatus{}, "", ""});
            }
            for (auto& worker : workers_) {
                if (worker.joinable()) {
                    worker.join();
                }
            }
        }
    }

    int get_result() const {
        return result_.load(std::memory_order_acquire);
    }

private:
    void worker_loop() {
        while (running_.load(std::memory_order_acquire)) {
            ProcessRowTask task;
            queue_.pop(task);

            // Check for stop signal (null db pointer)
            if (task.db == nullptr) {
                break;
            }

            int r = 0;
            try {
                r = task.db->process_row(task.id, task.filename.c_str(), task.status, task.hash_algo.c_str(), task.hash_hex.c_str());
            } catch (const std::exception& e) {
                r = 1; // count errors
                BasicLogger::instance().err("Exception processing %s - %s", task.filename.c_str(), e.what());
            }

            result_.fetch_add(r, std::memory_order_relaxed);
            pending_tasks_.fetch_sub(1, std::memory_order_release);
        }
    }

    rigtorp::MPMCQueue<ProcessRowTask> queue_;
    std::vector<std::thread> workers_;
    std::atomic<int> result_;
    std::atomic<bool> running_;
    std::atomic<size_t> pending_tasks_;
};
