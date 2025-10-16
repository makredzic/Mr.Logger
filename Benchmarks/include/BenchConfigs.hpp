#pragma once

#include "MR/Logger/Logger.hpp"
#include <MR/Logger/Config.hpp>
#include <MR/Queue/StdQueue.hpp>
#include <MR/Queue/FixedSizeBlockingQueue.hpp>
#include <MR/Logger/WriteRequest.hpp>
#include <memory>
#include <string>

namespace MR::Benchmarks {

enum class BenchmarkType {
    MRLogger,
    Spdlog
};

struct BenchmarkConfig {
    BenchmarkType type;
    std::string name;
    size_t thread_count;
    size_t total_messages;
    
    MR::Logger::Config logger_config;
    std::string spdlog_file_name;
    
    BenchmarkConfig(BenchmarkType t, const std::string& n, size_t threads = 1, size_t messages = 1000000)
        : type(t), name(n), thread_count(threads), total_messages(messages) {}
};

class BenchConfigs {
public:
    static BenchmarkConfig get_default_config(size_t thread_count = 1) {
        auto config = BenchmarkConfig(BenchmarkType::MRLogger, "Default", thread_count);

        auto default_logger_config = MR::Logger::Logger::defaultConfig();
        std::string thread_suffix = thread_count > 1 ? "_MultiThread" : "_SingleThread";
        default_logger_config.log_file_name = "Bench_Default" + thread_suffix + ".log";
        default_logger_config.max_log_size_bytes = 200 * 1024 * 1024; // 200MB - prevent rotation during benchmark
        default_logger_config.shutdown_timeout_seconds = 60u;

        config.logger_config = default_logger_config;

        return config;
    }
    
    static BenchmarkConfig get_small_config(size_t thread_count = 1) {
        auto config = BenchmarkConfig(BenchmarkType::MRLogger, "Small", thread_count);
        std::string thread_suffix = thread_count > 1 ? "_MultiThread" : "_SingleThread";
        config.logger_config = MR::Logger::Config{
            .log_file_name = "Bench_Small" + thread_suffix + ".log",
            .max_log_size_bytes = 200 * 1024 * 1024, // 200MB - prevent rotation during benchmark
            .batch_size = 32u,
            .queue_depth = 256u,
            .small_buffer_pool_size = 128u,
            .medium_buffer_pool_size = 64u,
            .large_buffer_pool_size = 32u,
            .small_buffer_size = 1024u,
            .medium_buffer_size = 4096u,
            .large_buffer_size = 16384u,
            .shutdown_timeout_seconds = 60u,
            ._queue = std::make_shared<MR::Queue::StdQueue<MR::Logger::WriteRequest>>(),
        };
        return config;
    }
    
    static BenchmarkConfig get_large_config(size_t thread_count = 1) {
        auto config = BenchmarkConfig(BenchmarkType::MRLogger, "Large", thread_count);
        std::string thread_suffix = thread_count > 1 ? "_MultiThread" : "_SingleThread";
        config.logger_config = MR::Logger::Config{
            .log_file_name = "Bench_Large" + thread_suffix + ".log",
            .max_log_size_bytes = 200 * 1024 * 1024, // 200MB - prevent rotation during benchmark
            .batch_size = 128u,
            .queue_depth = 4096u,
            .small_buffer_pool_size = 512u,
            .medium_buffer_pool_size = 256u,
            .large_buffer_pool_size = 128u,
            .small_buffer_size = 1024u,
            .medium_buffer_size = 4096u,
            .large_buffer_size = 16384u,
            .shutdown_timeout_seconds = 60u,
            ._queue = std::make_shared<MR::Queue::StdQueue<MR::Logger::WriteRequest>>(),
        };
        return config;
    }
    
    static BenchmarkConfig get_no_batch_config(size_t thread_count = 1) {
        auto config = BenchmarkConfig(BenchmarkType::MRLogger, "NoBatch", thread_count);
        std::string thread_suffix = thread_count > 1 ? "_MultiThread" : "_SingleThread";
        config.logger_config = MR::Logger::Config{
            .log_file_name = "Bench_NoBatch" + thread_suffix + ".log",
            .max_log_size_bytes = 200 * 1024 * 1024, // 200MB - prevent rotation during benchmark
            .batch_size = 1u,  // Submit each log immediately to io_uring
            .queue_depth = 512u,
            .small_buffer_pool_size = 512u,
            .medium_buffer_pool_size = 256u,
            .large_buffer_pool_size = 128u,
            .small_buffer_size = 1024u,
            .medium_buffer_size = 4096u,
            .large_buffer_size = 16384u,
            .shutdown_timeout_seconds = 60u,
            ._queue = std::make_shared<MR::Queue::StdQueue<MR::Logger::WriteRequest>>(),
        };
        return config;
    }

    static BenchmarkConfig get_spdlog_config(size_t thread_count = 1) {
        auto config = BenchmarkConfig(BenchmarkType::Spdlog, "Spdlog", thread_count);
        std::string thread_suffix = thread_count > 1 ? "_MultiThread" : "_SingleThread";
        config.spdlog_file_name = "Bench_Spdlog" + thread_suffix + ".log";
        return config;
    }

    // FixedSizeBlockingQueue benchmarks
    static BenchmarkConfig get_fixed_default_config(size_t thread_count = 1) {
        auto config = BenchmarkConfig(BenchmarkType::MRLogger, "FixedDefault", thread_count);
        std::string thread_suffix = thread_count > 1 ? "_MultiThread" : "_SingleThread";
        config.logger_config = MR::Logger::Config{
            .log_file_name = "Bench_Fixed_Default" + thread_suffix + ".log",
            .max_log_size_bytes = 200 * 1024 * 1024, // 200MB - prevent rotation during benchmark
            .batch_size = 64u,
            .queue_depth = 512u,
            .small_buffer_pool_size = 256u,
            .medium_buffer_pool_size = 128u,
            .large_buffer_pool_size = 64u,
            .small_buffer_size = 1024u,
            .medium_buffer_size = 4096u,
            .large_buffer_size = 16384u,
            .shutdown_timeout_seconds = 60u,
            ._queue = std::make_shared<MR::Queue::FixedSizeBlockingQueue<MR::Logger::WriteRequest>>(1024),
        };
        return config;
    }

    static BenchmarkConfig get_fixed_small_config(size_t thread_count = 1) {
        auto config = BenchmarkConfig(BenchmarkType::MRLogger, "FixedSmall", thread_count);
        std::string thread_suffix = thread_count > 1 ? "_MultiThread" : "_SingleThread";
        config.logger_config = MR::Logger::Config{
            .log_file_name = "Bench_Fixed_Small" + thread_suffix + ".log",
            .max_log_size_bytes = 200 * 1024 * 1024, // 200MB - prevent rotation during benchmark
            .batch_size = 32u,
            .queue_depth = 256u,
            .small_buffer_pool_size = 128u,
            .medium_buffer_pool_size = 64u,
            .large_buffer_pool_size = 32u,
            .small_buffer_size = 1024u,
            .medium_buffer_size = 4096u,
            .large_buffer_size = 16384u,
            .shutdown_timeout_seconds = 60u,
            ._queue = std::make_shared<MR::Queue::FixedSizeBlockingQueue<MR::Logger::WriteRequest>>(512),
        };
        return config;
    }

    static BenchmarkConfig get_fixed_large_config(size_t thread_count = 1) {
        auto config = BenchmarkConfig(BenchmarkType::MRLogger, "FixedLarge", thread_count);
        std::string thread_suffix = thread_count > 1 ? "_MultiThread" : "_SingleThread";
        config.logger_config = MR::Logger::Config{
            .log_file_name = "Bench_Fixed_Large" + thread_suffix + ".log",
            .max_log_size_bytes = 200 * 1024 * 1024, // 200MB - prevent rotation during benchmark
            .batch_size = 128u,
            .queue_depth = 4096u,
            .small_buffer_pool_size = 512u,
            .medium_buffer_pool_size = 256u,
            .large_buffer_pool_size = 128u,
            .small_buffer_size = 1024u,
            .medium_buffer_size = 4096u,
            .large_buffer_size = 16384u,
            .shutdown_timeout_seconds = 60u,
            ._queue = std::make_shared<MR::Queue::FixedSizeBlockingQueue<MR::Logger::WriteRequest>>(8192),
        };
        return config;
    }
};

}