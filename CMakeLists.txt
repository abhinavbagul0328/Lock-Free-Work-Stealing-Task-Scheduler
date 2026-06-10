cmake_minimum_required(VERSION 3.14)
project(WorkStealingScheduler VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Optimization and warning flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror -O3 -march=native")

# Threads
find_package(Threads REQUIRED)

# Include directory
include_directories(include)

# Define executable
add_executable(scheduler_benchmark 
    src/main.cpp
    src/benchmarks/parallel_fibonacci.cpp
    src/benchmarks/matrix_multiplication.cpp
    src/benchmarks/parallel_quicksort.cpp
)

# Link pthread
target_link_libraries(scheduler_benchmark PRIVATE Threads::Threads)
