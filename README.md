# Lock-Free Work-Stealing Task Scheduler

A high-performance parallel runtime designed to efficiently distribute workloads across multiple CPU cores while minimizing synchronization overhead. The scheduler implements dynamic load balancing through lock-free work stealing, allowing idle worker threads to steal tasks from busy workers without relying on traditional mutex-based locking mechanisms.

This project demonstrates advanced concepts in modern C++ concurrency, including lock-free programming, atomic memory ordering, and cache-aware data structures.

## Features

- **Lock-Free Synchronization:** Utilizes C++11/20 `std::atomic` with precise memory ordering (`memory_order_relaxed`, `memory_order_release`, `memory_order_acquire`, `memory_order_seq_cst`) to eliminate kernel locks and blocking.
- **Chase-Lev Work-Stealing Deque:** Each worker thread owns a double-ended queue. The owner pushes and pops tasks from the bottom (LIFO) for maximum cache locality, while other idle workers steal from the top (FIFO).
- **Cache Optimization:** Critical atomic variables are aligned to 64-byte boundaries (`alignas(64)`) to completely prevent false sharing and reduce cache-coherency traffic.
- **Dynamic Load Balancing:** Automatically balances highly irregular and recursive workloads across all available CPU cores.

## Project Structure

```text
.
├── CMakeLists.txt
├── include/
│   └── scheduler/
│       ├── metrics.hpp              # Performance tracking metrics
│       ├── scheduler.hpp            # Main thread pool and orchestrator
│       ├── task.hpp                 # Type-erased task abstraction
│       ├── work_stealing_deque.hpp  # Lock-free deque implementation
│       └── worker.hpp               # Worker thread logic
└── src/
    ├── main.cpp                     # Entry point and metric reporting
    └── benchmarks/                  # Benchmark implementations
        ├── benchmarks.hpp
        ├── matrix_multiplication.cpp
        ├── parallel_fibonacci.cpp
        └── parallel_quicksort.cpp
```

## Benchmarks Included

1. **Parallel Fibonacci:** A heavily recursive task generator that tests scheduler scalability, task spawning overhead, and deep work-stealing efficiency.
2. **Matrix Multiplication (1024x1024):** Tests compute-heavy, block-partitioned workloads.
3. **Parallel QuickSort (10M elements):** A recursive array partitioning benchmark that tests dynamic load balancing on uneven sub-tasks.

## Build Instructions

### Prerequisites
- A modern C++ compiler supporting **C++20** (e.g., GCC 10+, Clang 10+).
- **CMake** 3.14 or higher.

### Building

```bash
mkdir build
cd build
cmake ..
make -j$(nproc)
```

### Running

Execute the compiled benchmark suite from the build directory:

```bash
./scheduler_benchmark
```

## Example Output

Running on a 16-core machine yields phenomenal scalability:

```text
Starting Lock-Free Work-Stealing Scheduler with 16 workers.

--- Parallel Fibonacci Benchmark ---
Fibonacci(30) = 832040
Time: 0.00125155 seconds

--- Parallel Matrix Multiplication Benchmark ---
Matrix Multiplication (1024x1024) completed.
Sample C[0] = 2048
Time: 1.01848 seconds

--- Parallel QuickSort Benchmark ---
QuickSort (10000000 elements) completed. Is sorted? Yes
Time: 0.322263 seconds

--- Scheduler Metrics ---
Worker 0: Executed 260, Stolen 64
Worker 1: Executed 706, Stolen 91
Worker 2: Executed 244, Stolen 68
Worker 3: Executed 406, Stolen 64
Worker 4: Executed 451, Stolen 78
Worker 5: Executed 1056, Stolen 94
Worker 6: Executed 826, Stolen 78
Worker 7: Executed 686, Stolen 27
Worker 8: Executed 277, Stolen 58
Worker 9: Executed 201, Stolen 67
Worker 10: Executed 251, Stolen 53
Worker 11: Executed 868, Stolen 74
Worker 12: Executed 1232, Stolen 67
Worker 13: Executed 845, Stolen 77
Worker 14: Executed 464, Stolen 75
Worker 15: Executed 447, Stolen 59

Total Tasks Executed: 9220
Total Tasks Stolen: 1094
Steal Ratio: 11.8655%
```

## Key Learning Outcomes
This project serves as an educational reference for understanding OS scheduling limits, false sharing mitigation, parallel runtime design, and high-performance systems engineering.
