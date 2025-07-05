#pragma once

#include <MR/Logger/Config.hpp>
#include <MR/Queue/StdQueue.hpp>
#include <MR/Logger/WriteRequest.hpp>
#include <memory>

namespace MR::Benchmarks {

class BenchmarkConfigs {
public:
    // Default configuration (same as Logger's default)
    static MR::Logger::Config get_default_config() {
        return MR::Logger::Config{
            .log_file_name = "benchmark_default.log",
            .info_file_name = "",
            .warn_file_name = "",
            .error_file_name = "",
            .queue_depth = 256u,
            .batch_size = 10u,
            .max_logs_per_iteration = 10u,
            ._queue = std::make_shared<MR::Queue::StdQueue<MR::Logger::WriteRequest>>(),
        };
    }
    
    // Small buffer configuration - reduced values
    static MR::Logger::Config get_small_config() {
        return MR::Logger::Config{
            .log_file_name = "benchmark_small.log",
            .info_file_name = "",
            .warn_file_name = "",
            .error_file_name = "",
            .queue_depth = 64u,
            .batch_size = 5u,
            .max_logs_per_iteration = 5u,
            ._queue = std::make_shared<MR::Queue::StdQueue<MR::Logger::WriteRequest>>(),
        };
    }
    
    // Large buffer configuration - increased values
    static MR::Logger::Config get_large_config() {
        return MR::Logger::Config{
            .log_file_name = "benchmark_large.log",
            .info_file_name = "",
            .warn_file_name = "",
            .error_file_name = "",
            .queue_depth = 512u,
            .batch_size = 20u,
            .max_logs_per_iteration = 20u,
            ._queue = std::make_shared<MR::Queue::StdQueue<MR::Logger::WriteRequest>>(),
        };
    }
};

}