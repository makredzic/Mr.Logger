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
            .small_buffer_pool_size = 128u,
            .medium_buffer_pool_size = 64u,
            .large_buffer_pool_size = 32u,
            .small_buffer_size = 1024u,
            .medium_buffer_size = 4096u,
            .large_buffer_size = 16384u,
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
            .small_buffer_pool_size = 64u,
            .medium_buffer_pool_size = 32u,
            .large_buffer_pool_size = 16u,
            .small_buffer_size = 1024u,
            .medium_buffer_size = 4096u,
            .large_buffer_size = 16384u,
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
            .small_buffer_pool_size = 256u,
            .medium_buffer_pool_size = 128u,
            .large_buffer_pool_size = 64u,
            .small_buffer_size = 1024u,
            .medium_buffer_size = 4096u,
            .large_buffer_size = 16384u,
            ._queue = std::make_shared<MR::Queue::StdQueue<MR::Logger::WriteRequest>>(),
        };
    }
};

}