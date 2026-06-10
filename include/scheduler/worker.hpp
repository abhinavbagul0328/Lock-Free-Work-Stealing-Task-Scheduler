#pragma once

#include "task.hpp"
#include "metrics.hpp"
#include "work_stealing_deque.hpp"
#include <thread>

namespace scheduler {

class Scheduler;

/**
 * @brief Represents a worker thread that executes tasks.
 */
class alignas(64) Worker {
public:
    WorkStealingDeque<Task> deque;
    Metrics metrics;
    std::thread thread;
    size_t id;
    Scheduler* scheduler;

    Worker(size_t id, Scheduler* sched) : id(id), scheduler(sched) {}

    Worker(const Worker&) = delete;
    Worker& operator=(const Worker&) = delete;

    void start();
    void join() { if (thread.joinable()) thread.join(); }
    
private:
    void run();
};

} // namespace scheduler
