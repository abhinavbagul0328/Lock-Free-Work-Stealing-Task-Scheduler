#include "benchmarks.hpp"
#include <atomic>
#include <iostream>
#include <chrono>

using namespace scheduler;

int fib_serial(int n) {
    if (n < 2) return n;
    return fib_serial(n - 1) + fib_serial(n - 2);
}

void fib_parallel(int n, int* result, std::atomic<int>* pending_tasks) {
    if (n < 15) { // Cutoff for serial execution to avoid task overhead
        *result = fib_serial(n);
        pending_tasks->fetch_sub(1, std::memory_order_release);
        return;
    }

    int res1 = 0;
    int res2 = 0;
    std::atomic<int> local_pending{2};

    auto task1 = [n, &res1, &local_pending]() {
        fib_parallel(n - 1, &res1, &local_pending);
    };
    
    auto task2 = [n, &res2, &local_pending]() {
        fib_parallel(n - 2, &res2, &local_pending);
    };

    if (current_worker) {
        current_worker->deque.push_bottom(task1);
        current_worker->deque.push_bottom(task2);
    }

    // Wait for children to complete
    while (local_pending.load(std::memory_order_acquire) > 0) {
        if (current_worker) {
            auto t = current_worker->deque.pop_bottom();
            if (t) {
                (*t)();
                current_worker->metrics.tasks_executed++;
            } else {
                // Try to steal
                bool stole = false;
                if (current_worker->scheduler->num_workers() > 1) {
                    std::random_device rd;
                    std::mt19937 rng(rd());
                    std::uniform_int_distribution<size_t> dist(0, current_worker->scheduler->num_workers() - 1);
                    for (int i = 0; i < 4; ++i) {
                        size_t victim_id = dist(rng);
                        if (victim_id == current_worker->id) continue;
                        auto stolen_t = current_worker->scheduler->get_worker(victim_id)->deque.steal_top();
                        if (stolen_t) {
                            (*stolen_t)();
                            current_worker->metrics.tasks_executed++;
                            current_worker->metrics.tasks_stolen++;
                            stole = true;
                            break;
                        }
                    }
                }
                if (!stole) {
                    Task global_t;
                    if (current_worker->scheduler->steal_global(global_t)) {
                        global_t();
                        current_worker->metrics.tasks_executed++;
                    } else {
                        std::this_thread::yield();
                    }
                }
            }
        } else {
            std::this_thread::yield();
        }
    }

    *result = res1 + res2;
    pending_tasks->fetch_sub(1, std::memory_order_release);
}

namespace benchmarks {
    void run_parallel_fibonacci(Scheduler& sched) {
        std::cout << "--- Parallel Fibonacci Benchmark ---\n";
        int n = 30; 
        int result = 0;
        std::atomic<int> root_pending{1};
        
        auto start = std::chrono::high_resolution_clock::now();
        
        sched.submit([n, &result, &root_pending]() {
            fib_parallel(n, &result, &root_pending);
        });

        // Main thread waits for root task
        while (root_pending.load(std::memory_order_acquire) > 0) {
            std::this_thread::yield();
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> diff = end - start;
        
        std::cout << "Fibonacci(" << n << ") = " << result << "\n";
        std::cout << "Time: " << diff.count() << " seconds\n\n";
    }
}
