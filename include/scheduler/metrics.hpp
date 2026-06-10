#pragma once

#include <cstdint>

namespace scheduler {

/**
 * @brief Performance metrics for a single worker thread.
 * 
 * Aligned to 64 bytes to prevent false sharing between worker threads
 * updating their individual metrics.
 */
struct alignas(64) Metrics {
    uint64_t tasks_executed = 0;
    uint64_t tasks_stolen = 0;
    // Potentially add more: active_time, context_switches, etc.
};

} // namespace scheduler
