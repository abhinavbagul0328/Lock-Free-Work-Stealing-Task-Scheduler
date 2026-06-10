#pragma once

#include "../include/scheduler/scheduler.hpp"

namespace benchmarks {
    void run_parallel_fibonacci(scheduler::Scheduler& sched);
    void run_matrix_multiplication(scheduler::Scheduler& sched);
    void run_parallel_quicksort(scheduler::Scheduler& sched);
}
