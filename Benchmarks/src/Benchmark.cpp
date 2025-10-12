#include "Benchmark.hpp"
#include "BenchConfigs.hpp"
#include <MR/Logger/Logger.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/logger.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <iostream>
#include <fstream>
#include <chrono>
#include <string>
#include <filesystem>
#include <thread>
#include <vector>
#include <barrier>

using namespace std::chrono;

namespace MR::Benchmarks {

void deleteIfExists(const std::string& filename) {
    std::filesystem::path filePath = std::filesystem::current_path() / filename;

    if (std::filesystem::exists(filePath)) {
        std::filesystem::remove(filePath);
    }
}

// Only used for spdlog benchmarks - MRLogger uses flush() instead
void waitForLineCount(const std::string& filepath, size_t expected_lines, std::chrono::seconds timeout) {
    auto start = high_resolution_clock::now();
    auto timeout_point = start + timeout;

    while (high_resolution_clock::now() < timeout_point) {
        if (std::filesystem::exists(filepath)) {
            std::ifstream file(filepath);
            if (file.is_open()) {
                size_t line_count = 0;
                std::string line;
                while (std::getline(file, line)) {
                    ++line_count;
                }
                file.close();

                if (line_count >= expected_lines) {
                    return;
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    throw std::runtime_error("Timeout waiting for log file to reach expected line count");
}

std::string get_next_filename(const std::string& base_name, const std::string& extension) {
    std::string results_dir{"build/BenchmarkResults"};
    int counter = 1;
    std::string filename;
    
    do {
        filename = results_dir + "/" + base_name + std::to_string(counter) + extension;
        counter++;
    } while (std::filesystem::exists(filename));
    
    return filename;
}

void save_results_to_json(const BenchmarkResult& result) {
    std::string results_dir{"build/BenchmarkResults"};
    std::filesystem::create_directories(results_dir);

    std::string filename = get_next_filename(result.benchmark_name, ".json");

    std::ofstream json_file(filename);
    json_file << "{\n";
    json_file << "  \"benchmark_name\": \"" << result.benchmark_name << "\",\n";
    json_file << "  \"threads\": " << result.thread_count << ",\n";
    json_file << "  \"queue_time_ns\": " << result.duration.count() << ",\n";
    json_file << "  \"queue_time_ms\": " << (result.duration.count() / 1e6) << ",\n";
    json_file << "  \"end_to_end_time_ns\": " << result.end_to_end_duration.count() << ",\n";
    json_file << "  \"end_to_end_time_ms\": " << (result.end_to_end_duration.count() / 1e6) << ",\n";
    json_file << "  \"messages_logged\": " << result.total_msgs_logged << ",\n";
    json_file << "  \"queue_messages_per_second\": " << result.messages_per_second << ",\n";
    json_file << "  \"end_to_end_messages_per_second\": " << result.end_to_end_messages_per_second << ",\n";
    json_file << "  \"log_file_name\": \"" << result.log_file_name << "\",\n";

    if (result.config_details.spdlog.is_spdlog) {
        json_file << "  \"logger_type\": \"spdlog\"\n";
    } else {
        json_file << "  \"logger_type\": \"mrlogger\",\n";
        json_file << "  \"configuration\": {\n";
        json_file << "    \"queue_depth\": " << result.config_details.mrlogger.queue_depth << ",\n";
        json_file << "    \"batch_size\": " << result.config_details.mrlogger.batch_size << ",\n";
        json_file << "    \"max_logs_per_iteration\": " << result.config_details.mrlogger.max_logs_per_iteration << "\n";
        json_file << "  }\n";
    }

    json_file << "}\n";
    json_file.close();
}

struct DurationPair {
    nanoseconds queue_time;
    nanoseconds end_to_end_time;
};

nanoseconds measureSingleThreaded(std::shared_ptr<MR::Logger::Logger> logger, size_t msgs_per_thread) {
    auto start = high_resolution_clock::now();
    for (size_t i = 1; i <= msgs_per_thread; ++i) {
        logger->info("Benchmark message #{}", i);
    }
    auto queue_end = high_resolution_clock::now();

    return duration_cast<nanoseconds>(queue_end - start);
}

nanoseconds measureMultiThreaded(std::shared_ptr<MR::Logger::Logger> logger, size_t msgs_per_thread, size_t thread_count) {

    std::barrier sync_point(static_cast<std::ptrdiff_t>(thread_count) + 1);
    std::vector<std::thread> threads;
    threads.reserve(thread_count + 1);

    for (size_t thread_id = 0; thread_id < thread_count; ++thread_id) {
        threads.emplace_back([&sync_point, logger, msgs_per_thread]() {
            sync_point.arrive_and_wait();

            for (size_t i = 1; i <= msgs_per_thread; ++i) {
                logger->info("Benchmark #{}", i);
            }
        });
    }

    auto start = high_resolution_clock::now();
    sync_point.arrive_and_wait();

    for (auto& thread : threads) {
        thread.join();
    }

    auto queue_end = high_resolution_clock::now();

    return duration_cast<nanoseconds>(queue_end - start);
}

nanoseconds measureSpdLoggerSingleThreaded(std::shared_ptr<spdlog::logger> logger, size_t msgs_per_thread) {
    auto start = high_resolution_clock::now();

    for (size_t i = 1; i <= msgs_per_thread; ++i) {
        logger->info("Benchmark message #{}", i);
    }

    auto queue_end = high_resolution_clock::now();

    return duration_cast<nanoseconds>(queue_end - start);
}

nanoseconds measureSpdLoggerMultiThreaded(std::shared_ptr<spdlog::logger> logger, size_t msgs_per_thread, size_t thread_count) {

    std::barrier sync_point(static_cast<std::ptrdiff_t>(thread_count) + 1);
    std::vector<std::thread> threads;
    threads.reserve(thread_count + 1);

    for (size_t thread_id = 0; thread_id < thread_count; ++thread_id) {
        threads.emplace_back([&sync_point, logger, msgs_per_thread]() {
            sync_point.arrive_and_wait();

            for (size_t i = 1; i <= msgs_per_thread; ++i) {
                logger->info("Benchmark #{}", i);
            }
        });
    }

    auto start = high_resolution_clock::now();
    sync_point.arrive_and_wait();

    for (auto& thread : threads) {
        thread.join();
    }

    auto queue_end = high_resolution_clock::now();

    return duration_cast<nanoseconds>(queue_end - start);
}


BenchmarkResult run_mrlogger_benchmark(const BenchmarkConfig& config) {

    deleteIfExists(config.logger_config.log_file_name);

    const size_t msgs_per_thread = config.total_messages / config.thread_count;

    MR::Logger::init(config.logger_config);
    auto logger = MR::Logger::Logger::get();

    auto measurement_start = high_resolution_clock::now();

    nanoseconds queue_time;
    if (config.thread_count == 1) {
        queue_time = measureSingleThreaded(logger, msgs_per_thread);
    } else {
        queue_time = measureMultiThreaded(logger, msgs_per_thread, config.thread_count);
    }

    // Flush to ensure all log messages are written to disk
    logger->flush();
    auto measurement_end = high_resolution_clock::now();

    DurationPair durations{
        .queue_time = queue_time,
        .end_to_end_time = duration_cast<nanoseconds>(measurement_end - measurement_start)
    };

    double queue_seconds = static_cast<double>(durations.queue_time.count()) / 1e9;
    double end_to_end_seconds = static_cast<double>(durations.end_to_end_time.count()) / 1e9;
    double queue_messages_per_second = config.total_messages / queue_seconds;
    double end_to_end_messages_per_second = config.total_messages / end_to_end_seconds;

    std::cout << config.name << " (queue): " << (durations.queue_time.count() / 1e6) << " ms" << std::endl;
    std::cout << config.name << " (end-to-end): " << (durations.end_to_end_time.count() / 1e6) << " ms" << std::endl;

    BenchmarkResult result{
        .duration = durations.queue_time,
        .end_to_end_duration = durations.end_to_end_time,
        .total_msgs_logged = config.total_messages,
        .msgs_per_thread = msgs_per_thread,
        .messages_per_second = queue_messages_per_second,
        .end_to_end_messages_per_second = end_to_end_messages_per_second,
        .benchmark_name = config.name,
        .log_file_name = config.logger_config.log_file_name,
        .thread_count = config.thread_count,
        .config_details = {}
    };

    result.config_details.mrlogger.queue_depth = config.logger_config.queue_depth;
    result.config_details.mrlogger.batch_size = config.logger_config.batch_size;
    result.config_details.mrlogger.max_logs_per_iteration = logger->getMaxLogsPerIteration();

    return result;
}

BenchmarkResult run_spdlog_benchmark(const BenchmarkConfig& config) {

    deleteIfExists(config.spdlog_file_name);

    const size_t msgs_per_thread = config.total_messages / config.thread_count;
    const std::chrono::seconds timeout(90); // Reasonable timeout for spdlog

    auto measurement_start = high_resolution_clock::now();

    nanoseconds queue_time;

    if (config.thread_count == 1) {

        auto logger = spdlog::basic_logger_st("benchmark_logger", config.spdlog_file_name);
        logger->set_level(spdlog::level::info);

        queue_time = measureSpdLoggerSingleThreaded(logger, msgs_per_thread);

        // Flush to ensure all logs are written
        logger->flush();

        spdlog::drop("benchmark_logger");

    } else {

        auto logger = spdlog::basic_logger_mt("benchmark_logger_mt", config.spdlog_file_name);
        logger->set_level(spdlog::level::info);

        queue_time = measureSpdLoggerMultiThreaded(logger, msgs_per_thread, config.thread_count);

        // Flush to ensure all logs are written
        logger->flush();

        spdlog::drop("benchmark_logger_mt");
    }

    // Wait for file to reach expected line count
    waitForLineCount(config.spdlog_file_name, config.total_messages, timeout);
    auto measurement_end = high_resolution_clock::now();

    DurationPair durations{
        .queue_time = queue_time,
        .end_to_end_time = duration_cast<nanoseconds>(measurement_end - measurement_start)
    };


    double queue_seconds = static_cast<double>(durations.queue_time.count()) / 1e9;
    double end_to_end_seconds = static_cast<double>(durations.end_to_end_time.count()) / 1e9;
    double queue_messages_per_second = config.total_messages / queue_seconds;
    double end_to_end_messages_per_second = config.total_messages / end_to_end_seconds;

    std::cout << config.name << " (queue): " << (durations.queue_time.count() / 1e6) << " ms" << std::endl;
    std::cout << config.name << " (end-to-end): " << (durations.end_to_end_time.count() / 1e6) << " ms" << std::endl;

    BenchmarkResult result{
        .duration = durations.queue_time,
        .end_to_end_duration = durations.end_to_end_time,
        .total_msgs_logged = config.total_messages,
        .msgs_per_thread = msgs_per_thread,
        .messages_per_second = queue_messages_per_second,
        .end_to_end_messages_per_second = end_to_end_messages_per_second,
        .benchmark_name = config.name,
        .log_file_name = config.spdlog_file_name,
        .thread_count = config.thread_count,
        .config_details = {}
    };

    result.config_details.spdlog.is_spdlog = true;

    return result;
}

BenchmarkResult run_benchmark(const BenchmarkConfig& config) {
    BenchmarkResult result;
    
    if (config.type == BenchmarkType::MRLogger) {
        result = run_mrlogger_benchmark(config);
    } else {
        result = run_spdlog_benchmark(config);
    }
    
    save_results_to_json(result);
    
    return result;
}

}