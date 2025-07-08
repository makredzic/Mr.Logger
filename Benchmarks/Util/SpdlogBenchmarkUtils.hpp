#pragma once

#include "SpdlogBenchmarkConfigs.hpp"
#include <chrono>
#include <string>

namespace MR::Benchmarks {

struct SpdlogBenchmarkResult {
    std::chrono::nanoseconds duration;
    size_t messages_logged;
    double messages_per_second;
    std::string benchmark_name;
    std::string log_file_name;
    size_t threads;
};

SpdlogBenchmarkResult benchmark_spdlog_performance(const SpdlogBenchmarkConfig& config, const std::string& benchmark_name);
SpdlogBenchmarkResult benchmark_spdlog_performance_multithreaded(const SpdlogBenchmarkConfig& config, const std::string& benchmark_name, size_t num_threads = 10, size_t messages_per_thread = 100000);
void save_spdlog_results_to_json(const SpdlogBenchmarkResult& result);

void deleteIfExists(const std::string& filename);

}