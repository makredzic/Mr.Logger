#pragma once

#include <MR/Logger/Config.hpp>
#include <chrono>

namespace MR::Benchmarks {

struct BenchmarkResult {
    std::chrono::nanoseconds duration;
    size_t messages_logged;
    double messages_per_second;
};

BenchmarkResult benchmark_logger_performance(const MR::Logger::Config& config);

}