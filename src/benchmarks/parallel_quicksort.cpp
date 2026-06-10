#include "benchmarks.hpp"
#include <atomic>
#include <iostream>
#include <chrono>
#include <vector>
#include <algorithm>
#include <random>

using namespace scheduler;

void quicksort_serial(std::vector<int>& arr, int low, int high) {
    if (low < high) {
        int pivot = arr[high];
        int i = (low - 1);
        for (int j = low; j <= high - 1; j++) {
            if (arr[j] < pivot) {
                i++;
                std::swap(arr[i], arr[j]);
            }
        }
        std::swap(arr[i + 1], arr[high]);
        int pi = i + 1;

        quicksort_serial(arr, low, pi - 1);
        quicksort_serial(arr, pi + 1, high);
    }
}

void quicksort_parallel(std::vector<int>& arr, int low, int high, std::atomic<int>* pending_tasks) {
    if (low < high) {
        if (high - low < 10000) { // Cutoff for serial sorting
            quicksort_serial(arr, low, high);
            pending_tasks->fetch_sub(1, std::memory_order_release);
            return;
        }

        int pivot = arr[high];
        int i = (low - 1);
        for (int j = low; j <= high - 1; j++) {
            if (arr[j] < pivot) {
                i++;
                std::swap(arr[i], arr[j]);
            }
        }
        std::swap(arr[i + 1], arr[high]);
        int pi = i + 1;

        std::atomic<int> local_pending{2};

        auto task1 = [&arr, low, pi, &local_pending]() {
            quicksort_parallel(arr, low, pi - 1, &local_pending);
        };
        auto task2 = [&arr, pi, high, &local_pending]() {
            quicksort_parallel(arr, pi + 1, high, &local_pending);
        };

        if (current_worker) {
            current_worker->deque.push_bottom(task1);
            current_worker->deque.push_bottom(task2);
        }

        while (local_pending.load(std::memory_order_acquire) > 0) {
            if (current_worker) {
                auto t = current_worker->deque.pop_bottom();
                if (t) {
                    (*t)();
                    current_worker->metrics.tasks_executed++;
                } else {
                    bool stole = false;
                    if (current_worker->scheduler->num_workers() > 1) {
                        std::random_device rd;
                        std::mt19937 rng(rd());
                        std::uniform_int_distribution<size_t> dist(0, current_worker->scheduler->num_workers() - 1);
                        for (int k = 0; k < 4; ++k) {
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
    }
    pending_tasks->fetch_sub(1, std::memory_order_release);
}

namespace benchmarks {
    void run_parallel_quicksort(Scheduler& sched) {
        std::cout << "--- Parallel QuickSort Benchmark ---\n";
        int N = 10000000; // 10 million elements
        std::vector<int> arr(N);
        std::mt19937 rng(42);
        std::uniform_int_distribution<int> dist(0, 1000000);
        for(int i=0; i<N; ++i) arr[i] = dist(rng);
        
        std::atomic<int> root_pending{1};
        
        auto start = std::chrono::high_resolution_clock::now();
        
        sched.submit([&arr, N, &root_pending]() {
            quicksort_parallel(arr, 0, N - 1, &root_pending);
        });

        while (root_pending.load(std::memory_order_acquire) > 0) {
            std::this_thread::yield();
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> diff = end - start;
        
        bool sorted = std::is_sorted(arr.begin(), arr.end());
        
        std::cout << "QuickSort (" << N << " elements) completed. Is sorted? " << (sorted ? "Yes" : "No") << "\n";
        std::cout << "Time: " << diff.count() << " seconds\n\n";
    }
}
