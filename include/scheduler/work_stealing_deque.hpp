#pragma once

#include <atomic>
#include <vector>
#include <optional>
#include <cstdint>

namespace scheduler {

/**
 * @brief Lock-Free Work-Stealing Deque (Chase-Lev)
 * 
 * Allows the owner thread to push/pop from the bottom (LIFO behavior),
 * while other threads can steal from the top (FIFO behavior).
 */
template<typename T>
class WorkStealingDeque {
    alignas(64) std::atomic<int64_t> top;
    alignas(64) std::atomic<int64_t> bottom;
    std::vector<T> buffer;
    int64_t mask;

public:
    explicit WorkStealingDeque(size_t capacity_power_of_2 = 65536)
        : top(0), bottom(0), buffer(capacity_power_of_2), mask(capacity_power_of_2 - 1) {
    }

    WorkStealingDeque(const WorkStealingDeque&) = delete;
    WorkStealingDeque& operator=(const WorkStealingDeque&) = delete;

    /**
     * @brief Pushes a task onto the bottom of the deque.
     * @param task The task to push.
     * @note Only called by the owner thread.
     */
    void push_bottom(T task) {
        int64_t b = bottom.load(std::memory_order_relaxed);
        buffer[b & mask] = std::move(task);
        std::atomic_thread_fence(std::memory_order_release);
        bottom.store(b + 1, std::memory_order_relaxed);
    }

    /**
     * @brief Pops a task from the bottom of the deque.
     * @return An optional containing the task if successful, or nullopt if empty.
     * @note Only called by the owner thread.
     */
    std::optional<T> pop_bottom() {
        int64_t b = bottom.load(std::memory_order_relaxed) - 1;
        bottom.store(b, std::memory_order_relaxed);
        std::atomic_thread_fence(std::memory_order_seq_cst);
        int64_t t = top.load(std::memory_order_relaxed);
        
        std::optional<T> result;
        if (t <= b) {
            // Non-empty queue
            // IMPORTANT: Do NOT use std::move here, because steal_top() might be copying
            // this exact same element concurrently!
            result = buffer[b & mask];
            if (t == b) {
                // Single last element in queue. Potential race with steal_top().
                if (!top.compare_exchange_strong(t, t + 1, 
                                                 std::memory_order_seq_cst, 
                                                 std::memory_order_relaxed)) {
                    // Lost race to steal. The queue is actually empty.
                    result = std::nullopt;
                }
                bottom.store(b + 1, std::memory_order_relaxed);
            }
        } else {
            // Empty queue
            bottom.store(b + 1, std::memory_order_relaxed);
        }
        return result;
    }

    /**
     * @brief Steals a task from the top of the deque.
     * @return An optional containing the task if successful, or nullopt if empty or contention.
     * @note Called by other threads trying to steal work.
     */
    std::optional<T> steal_top() {
        int64_t t = top.load(std::memory_order_acquire);
        std::atomic_thread_fence(std::memory_order_seq_cst);
        int64_t b = bottom.load(std::memory_order_acquire);
        
        std::optional<T> result;
        if (t < b) {
            // Non-empty queue
            result = buffer[t & mask];
            // Compete to steal the task at `t`
            if (!top.compare_exchange_strong(t, t + 1, 
                                             std::memory_order_seq_cst, 
                                             std::memory_order_relaxed)) {
                // Lost race to another stealer or pop_bottom()
                result = std::nullopt;
            }
        }
        return result;
    }
};

} // namespace scheduler
