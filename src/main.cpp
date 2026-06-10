#include <iostream>
#include <thread>
#include "../include/scheduler/scheduler.hpp"
#include "benchmarks/benchmarks.hpp"

int main() {
    size_t num_cores = std::thread::hardware_concurrency();
    if (num_cores == 0) num_cores = 4;
    
    std::cout << "Starting Lock-Free Work-Stealing Scheduler with " << num_cores << " workers.\n\n";

    scheduler::Scheduler sched(num_cores);
    sched.start();

    // Run Benchmarks
    benchmarks::run_parallel_fibonacci(sched);
    benchmarks::run_matrix_multiplication(sched);
    benchmarks::run_parallel_quicksort(sched);

    sched.stop();

    // Print Metrics
    std::cout << "--- Scheduler Metrics ---\n";
    uint64_t total_tasks = 0;
    uint64_t total_stolen = 0;
    
    for (size_t i = 0; i < sched.num_workers(); ++i) {
        auto worker = sched.get_worker(i);
        std::cout << "Worker " << worker->id 
                  << ": Executed " << worker->metrics.tasks_executed 
                  << ", Stolen " << worker->metrics.tasks_stolen << "\n";
                  
        total_tasks += worker->metrics.tasks_executed;
        total_stolen += worker->metrics.tasks_stolen;
    }
    
    std::cout << "\nTotal Tasks Executed: " << total_tasks << "\n";
    std::cout << "Total Tasks Stolen: " << total_stolen << "\n";
    if (total_tasks > 0) {
        std::cout << "Steal Ratio: " << (double)total_stolen / total_tasks * 100.0 << "%\n";
    }

    return 0;
}
