#include "BenchmarkUtils.hpp"
#include <MR/Logger/Logger.hpp>
#include <iostream>
#include <chrono>
#include <string>

namespace MR::Benchmarks {

BenchmarkResult benchmark_logger_performance(const MR::Logger::Config& config) {
    const size_t NUM_MESSAGES = 10000;
    
    // Create logger with specified config
    MR::Logger::Logger logger(config);
    
    // Start timing
    auto start = std::chrono::high_resolution_clock::now();
    
    // Log 10,000 messages
    for (size_t i = 1; i <= NUM_MESSAGES; ++i) {
        logger.info("Benchmark message #" + std::to_string(i));
    }
    
    // Stop timing
    auto end = std::chrono::high_resolution_clock::now();
    
    // Calculate duration
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    
    // Calculate messages per second
    double seconds = static_cast<double>(duration.count()) / 1e9;
    double messages_per_second = NUM_MESSAGES / seconds;
    
    // Print results
    std::cout << "Benchmark Results:" << std::endl;
    std::cout << "  Messages logged: " << NUM_MESSAGES << std::endl;
    std::cout << "  Duration: " << duration.count() << " ns" << std::endl;
    std::cout << "  Duration: " << duration.count() / 1e6 << " ms" << std::endl;
    std::cout << "  Messages per second: " << messages_per_second << std::endl;
    std::cout << "  Configuration:" << std::endl;
    std::cout << "    Queue depth: " << config.queue_depth << std::endl;
    std::cout << "    Batch size: " << config.batch_size << std::endl;
    std::cout << "    Max logs per iteration: " << config.max_logs_per_iteration << std::endl;
    std::cout << std::endl;
    
    return {duration, NUM_MESSAGES, messages_per_second};
}

}