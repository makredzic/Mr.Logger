#pragma once

#include <MR/Logger/Config.hpp>
#include <chrono>
#include <string>

namespace MR::Benchmarks {

struct BenchmarkResult {
    std::chrono::nanoseconds duration;
    size_t messages_logged;
    double messages_per_second;
    std::string benchmark_name;
    std::string log_file_name;
    uint16_t queue_depth;
    uint16_t batch_size;
    uint16_t max_logs_per_iteration;
};

BenchmarkResult benchmark_logger_performance(const MR::Logger::Config& config, const std::string& benchmark_name);
void save_results_to_json(const BenchmarkResult& result, const std::string& results_dir = "Results");

void deleteIfExists(const std::string& filename);

}