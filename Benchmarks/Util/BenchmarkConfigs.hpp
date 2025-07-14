#pragma once

#include "MR/Logger/Logger.hpp"
#include <MR/Logger/Config.hpp>
#include <MR/Queue/StdQueue.hpp>
#include <MR/Logger/WriteRequest.hpp>
#include <memory>

namespace MR::Benchmarks {

class BenchmarkConfigs {
public:
    // Default configuration (same as Logger's default)
    static MR::Logger::Config get_default_config() {
        return MR::Logger::Logger::defaultConfig();
    }
    
    // Small buffer configuration - optimized for low latency
    static MR::Logger::Config get_small_config() {
        return MR::Logger::Config{
            .log_file_name = "benchmark_small.log",
            .info_file_name = "",
            .warn_file_name = "",
            .error_file_name = "",
            .batch_size = 32u,
            .max_logs_per_iteration = 128u,
            .queue_depth = 256u,
            .small_buffer_pool_size = 128u,
            .medium_buffer_pool_size = 64u,
            .large_buffer_pool_size = 32u,
            .small_buffer_size = 1024u,
            .medium_buffer_size = 4096u,
            .large_buffer_size = 16384u,
            ._queue = std::make_shared<MR::Queue::StdQueue<MR::Logger::WriteRequest>>(),
        };
    }
    
    // Large buffer configuration - optimized for high throughput
    static MR::Logger::Config get_large_config() {
        return MR::Logger::Config{
            .log_file_name = "benchmark_large.log",
            .info_file_name = "",
            .warn_file_name = "",
            .error_file_name = "",
            .batch_size = 1024u,
            .max_logs_per_iteration = 2048u,
            .queue_depth = 4096u,
            .small_buffer_pool_size = 512u,
            .medium_buffer_pool_size = 256u,
            .large_buffer_pool_size = 128u,
            .small_buffer_size = 1024u,
            .medium_buffer_size = 4096u,
            .large_buffer_size = 16384u,
            ._queue = std::make_shared<MR::Queue::StdQueue<MR::Logger::WriteRequest>>(),
        };
    }
};

}