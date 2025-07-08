#include "SpdlogBenchmarkUtils.hpp"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <chrono>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <thread>
#include <barrier>
#include <vector>

namespace MR::Benchmarks {

void deleteIfExists(const std::string& filename) {
    std::filesystem::path filePath = std::filesystem::current_path() / filename;

    if (std::filesystem::exists(filePath)) {
        std::filesystem::remove(filePath);
    } 
}

std::string get_next_spdlog_filename(const std::string& base_name, const std::string& extension) {
    std::string results_dir{"BenchmarkResults"};
    int counter = 1;
    std::string filename;
    
    do {
        filename = results_dir + "/" + base_name + std::to_string(counter) + extension;
        counter++;
    } while (std::filesystem::exists(filename));
    
    return filename;
}

void save_spdlog_results_to_json(const SpdlogBenchmarkResult& result) {
    // Create results directory if it doesn't exist
    std::string results_dir{"BenchmarkResults"};
    std::filesystem::create_directories(results_dir);
    
    // Generate filename with incrementing number
    std::string filename = get_next_spdlog_filename(result.benchmark_name, ".json");
    
    std::ofstream json_file(filename);
    json_file << "{\n";
    json_file << "  \"benchmark_name\": \"" << result.benchmark_name << "\",\n";
    json_file << "  \"threads\": " << result.threads << ",\n";
    json_file << "  \"duration_ns\": " << result.duration.count() << ",\n";
    json_file << "  \"duration_ms\": " << (result.duration.count() / 1e6) << ",\n";
    json_file << "  \"messages_logged\": " << result.messages_logged << ",\n";
    json_file << "  \"messages_per_second\": " << result.messages_per_second << ",\n";
    json_file << "  \"log_file_name\": \"" << result.log_file_name << "\",\n";
    json_file << "  \"logger_type\": \"spdlog\"\n";
    json_file << "}\n";
    json_file.close();
}

SpdlogBenchmarkResult benchmark_spdlog_performance(const SpdlogBenchmarkConfig& config, const std::string& benchmark_name) {
    constexpr size_t MESSAGE_COUNT = 1000000;
    
    // Delete existing log file
    deleteIfExists(config.log_file_name);
    
    // Create spdlog logger
    auto logger = spdlog::basic_logger_st("benchmark_logger", config.log_file_name);
    logger->set_level(spdlog::level::info);
    logger->flush_on(spdlog::level::info);
    
    // Start timing
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Log messages
    for (size_t i = 1; i <= MESSAGE_COUNT; ++i) {
        logger->info("Benchmark message #{}", i);
    }
    
    // Force flush to ensure all messages are written
    logger->flush();
    
    // End timing
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
    
    // Calculate performance metrics
    double messages_per_second = static_cast<double>(MESSAGE_COUNT) / (duration.count() / 1e9);
    
    // Create result
    SpdlogBenchmarkResult result{
        .duration = duration,
        .messages_logged = MESSAGE_COUNT,
        .messages_per_second = messages_per_second,
        .benchmark_name = benchmark_name,
        .log_file_name = config.log_file_name,
        .threads = 1
    };
    
    // Print to console
    std::cout << benchmark_name << ": " << (duration.count() / 1e6) << " ms" << std::endl;
    
    // Save to JSON
    save_spdlog_results_to_json(result);
    
    // Clean up
    spdlog::drop("benchmark_logger");
    
    return result;
}

SpdlogBenchmarkResult benchmark_spdlog_performance_multithreaded(const SpdlogBenchmarkConfig& config, const std::string& benchmark_name, size_t num_threads, size_t messages_per_thread) {
    const size_t TOTAL_MESSAGES = num_threads * messages_per_thread;
    
    // Delete existing log file
    deleteIfExists(config.log_file_name);
    
    // Create thread-safe spdlog logger
    auto logger = spdlog::basic_logger_mt("benchmark_logger_mt", config.log_file_name);
    logger->set_level(spdlog::level::info);
    logger->flush_on(spdlog::level::info);
    
    // Create barrier for synchronization
    std::barrier sync_point(static_cast<std::ptrdiff_t>(num_threads));
    
    // Vector to store threads
    std::vector<std::thread> threads;
    threads.reserve(num_threads);
    
    // Start timing
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Create and start threads
    for (size_t t = 0; t < num_threads; ++t) {
        threads.emplace_back([&sync_point, messages_per_thread, logger]() {
            // Wait for all threads to be ready
            sync_point.arrive_and_wait();
            
            // Log messages
            for (size_t i = 1; i <= messages_per_thread; ++i) {
                logger->info("Benchmark #{}", i);
            }
        });
    }
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Force flush to ensure all messages are written
    logger->flush();
    
    // End timing
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
    
    // Calculate performance metrics
    double messages_per_second = static_cast<double>(TOTAL_MESSAGES) / (duration.count() / 1e9);
    
    // Create result
    SpdlogBenchmarkResult result{
        .duration = duration,
        .messages_logged = TOTAL_MESSAGES,
        .messages_per_second = messages_per_second,
        .benchmark_name = benchmark_name,
        .log_file_name = config.log_file_name,
        .threads = num_threads
    };
    
    // Print to console
    std::cout << benchmark_name << ": " << (duration.count() / 1e6) << " ms" << std::endl;
    
    // Save to JSON
    save_spdlog_results_to_json(result);
    
    // Clean up
    spdlog::drop("benchmark_logger_mt");
    
    return result;
}

}