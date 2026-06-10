#pragma once

#include <functional>

namespace scheduler {

/**
 * @brief Represents a unit of work that can be executed by the scheduler.
 */
class Task {
    std::function<void()> func;

public:
    Task() = default;
    Task(const Task&) = default;
    Task(Task&&) = default;
    Task& operator=(const Task&) = default;
    Task& operator=(Task&&) = default;
    
    template<typename F, typename = std::enable_if_t<!std::is_same_v<std::decay_t<F>, Task>>>
    Task(F&& f) : func(std::forward<F>(f)) {}

    void operator()() const {
        if (func) {
            func();
        }
    }

    explicit operator bool() const {
        return static_cast<bool>(func);
    }
};

} // namespace scheduler
