#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <memory>
#include <string>

namespace MR::Benchmarks {

struct SpdlogBenchmarkConfig {
    std::string log_file_name;
};

class SpdlogBenchmarkConfigs {
public:
    // Default configuration for single-threaded benchmark
    static SpdlogBenchmarkConfig get_default_config() {
        return SpdlogBenchmarkConfig{
            .log_file_name = "spdlog_benchmark.log"
        };
    }
    
    // Configuration for multi-threaded benchmark
    static SpdlogBenchmarkConfig get_multithread_config() {
        return SpdlogBenchmarkConfig{
            .log_file_name = "spdlog_benchmark_multithread.log"
        };
    }
};

}