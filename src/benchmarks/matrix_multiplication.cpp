#include "benchmarks.hpp"
#include <atomic>
#include <iostream>
#include <chrono>
#include <vector>

using namespace scheduler;

void mat_mul_block(const std::vector<float>& A, const std::vector<float>& B, std::vector<float>& C, 
                   int N, int row_start, int row_end, std::atomic<int>* pending_tasks) {
    for (int i = row_start; i < row_end; ++i) {
        for (int j = 0; j < N; ++j) {
            float sum = 0.0f;
            for (int k = 0; k < N; ++k) {
                sum += A[i * N + k] * B[k * N + j];
            }
            C[i * N + j] = sum;
        }
    }
    pending_tasks->fetch_sub(1, std::memory_order_release);
}

namespace benchmarks {
    void run_matrix_multiplication(Scheduler& sched) {
        std::cout << "--- Parallel Matrix Multiplication Benchmark ---\n";
        int N = 1024;
        std::vector<float> A(N * N, 1.0f);
        std::vector<float> B(N * N, 2.0f);
        std::vector<float> C(N * N, 0.0f);
        
        int num_blocks = 64; // Divide work into more blocks for better load balancing
        int block_size = N / num_blocks;
        
        std::atomic<int> root_pending{num_blocks};
        
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int b = 0; b < num_blocks; ++b) {
            int row_start = b * block_size;
            int row_end = (b == num_blocks - 1) ? N : (b + 1) * block_size;
            
            sched.submit([&A, &B, &C, N, row_start, row_end, &root_pending]() {
                mat_mul_block(A, B, C, N, row_start, row_end, &root_pending);
            });
        }

        // Main thread waits for all blocks
        while (root_pending.load(std::memory_order_acquire) > 0) {
            std::this_thread::yield();
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> diff = end - start;
        
        std::cout << "Matrix Multiplication (" << N << "x" << N << ") completed.\n";
        std::cout << "Sample C[0] = " << C[0] << "\n";
        std::cout << "Time: " << diff.count() << " seconds\n\n";
    }
}
