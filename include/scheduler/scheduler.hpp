#pragma once

#include "worker.hpp"
#include <vector>
#include <memory>
#include <atomic>
#include <random>
#include <mutex>
#include <deque>

namespace scheduler {

/**
 * @brief Manages worker threads and global task submission.
 */
class Scheduler {
    std::vector<std::unique_ptr<Worker>> workers;
    std::atomic<bool> running{false};

    // Global queue for root tasks submitted from outside the worker threads.
    std::mutex global_queue_mutex;
    std::deque<Task> global_queue;

public:
    Scheduler(size_t num_threads) {
        for (size_t i = 0; i < num_threads; ++i) {
            workers.push_back(std::make_unique<Worker>(i, this));
        }
    }

    ~Scheduler() {
        stop();
    }

    void start() {
        running.store(true, std::memory_order_release);
        for (auto& w : workers) {
            w->start();
        }
    }

    void stop() {
        if (running.exchange(false, std::memory_order_acquire)) {
            for (auto& w : workers) {
                w->join();
            }
        }
    }

    /**
     * @brief Submit a top-level task from an external thread.
     */
    void submit(Task task) {
        std::lock_guard<std::mutex> lock(global_queue_mutex);
        global_queue.push_back(std::move(task));
    }

    bool steal_global(Task& out_task) {
        std::lock_guard<std::mutex> lock(global_queue_mutex);
        if (!global_queue.empty()) {
            out_task = std::move(global_queue.front());
            global_queue.pop_front();
            return true;
        }
        return false;
    }

    Worker* get_worker(size_t index) { return workers[index].get(); }
    size_t num_workers() const { return workers.size(); }
    bool is_running() const { return running.load(std::memory_order_relaxed); }
};

// Inline implementations for Worker methods to avoid multiple definitions
// and allow mutual usage of Scheduler and Worker.

inline thread_local Worker* current_worker = nullptr;

inline void Worker::start() {
    thread = std::thread([this]() {
        current_worker = this;
        this->run();
    });
}

inline void Worker::run() {
    thread_local std::mt19937 rng(std::random_device{}());
    size_t num_workers = scheduler->num_workers();
    std::uniform_int_distribution<size_t> dist(0, num_workers - 1);

    while (scheduler->is_running()) {
        // 1. Try to pop from local deque
        auto task = deque.pop_bottom();
        if (task) {
            (*task)();
            metrics.tasks_executed++;
            continue;
        }

        // 2. Try to steal from the global root task queue
        Task global_task;
        if (scheduler->steal_global(global_task)) {
            global_task();
            metrics.tasks_executed++;
            continue;
        }

        // 3. If local and global are empty, try to steal from other workers
        bool stole = false;
        if (num_workers > 1) {
            for (int i = 0; i < 4; ++i) { // Try a few times
                size_t victim_id = dist(rng);
                if (victim_id == id) continue; // Don't steal from self

                auto stolen_task = scheduler->get_worker(victim_id)->deque.steal_top();
                if (stolen_task) {
                    (*stolen_task)();
                    metrics.tasks_executed++;
                    metrics.tasks_stolen++;
                    stole = true;
                    break;
                }
            }
        }

        // 4. Yield if no work was found
        if (!stole) {
            std::this_thread::yield();
        }
    }
}

} // namespace scheduler
