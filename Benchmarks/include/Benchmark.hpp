#pragma once

#include "BenchConfigs.hpp"
#include <chrono>
#include <string>

namespace MR::Benchmarks {

struct BenchmarkResult {
    std::chrono::nanoseconds duration;
    std::chrono::nanoseconds end_to_end_duration;
    size_t total_msgs_logged;
    size_t msgs_per_thread;
    double messages_per_second;
    double end_to_end_messages_per_second;
    std::string benchmark_name;
    std::string log_file_name;
    size_t thread_count;

    union {
        struct {
            uint16_t queue_depth;
            uint16_t batch_size;
            uint16_t max_logs_per_iteration;
        } mrlogger;

        struct {
            bool is_spdlog;
        } spdlog;
    } config_details;
};

BenchmarkResult run_benchmark(const BenchmarkConfig& config);

void save_results_to_json(const BenchmarkResult& result);

void deleteIfExists(const std::string& filename);

}