#pragma once

#include <chrono>

#define EXECUTION_TIME_PROFILER_START             \
    auto start = std::chrono::system_clock::now();

#define EXECUTION_TIME_PROFILER_END               \
    auto end = std::chrono::system_clock::now();  \
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

#define GET_EXECUTION_TIME_MS                     \
    elapsed.count() / 1000.f
