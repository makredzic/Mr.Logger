#include "BenchmarkUtils.hpp"

#include <MR/Logger/Logger.hpp>
#include <iostream>
#include <fstream>
#include <chrono>
#include <string>
#include <filesystem>
#include <thread>
#include <vector>
#include <barrier>

namespace MR::Benchmarks {

void waitForLogCompletion(int expected_messages, const std::string& log_file_name) {
    auto start = std::chrono::steady_clock::now();
    const std::chrono::seconds timeout{5};
    
    while (std::chrono::steady_clock::now() - start < timeout) {
        if (std::filesystem::exists(log_file_name)) {
            std::ifstream file(log_file_name);
            std::string line;
            int line_count = 0;
            
            while (std::getline(file, line)) {
                line_count++;
            }
            
            if (line_count >= expected_messages) {
                return;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}


void deleteIfExists(const std::string& filename) {
    std::filesystem::path filePath = std::filesystem::current_path() / filename;

    if (std::filesystem::exists(filePath)) {
        std::filesystem::remove(filePath);
    } 
}

std::string get_next_filename(const std::string& base_name, const std::string& extension) {
    std::string results_dir{"BenchmarkResults"};
    int counter = 1;
    std::string filename;
    
    do {
        filename = results_dir + "/" + base_name + std::to_string(counter) + extension;
        counter++;
    } while (std::filesystem::exists(filename));
    
    return filename;
}

void save_results_to_json(const BenchmarkResult& result, int num_of_threads) {
    // Create results directory if it doesn't exist
    std::string results_dir{"BenchmarkResults"};
    std::filesystem::create_directories(results_dir);
    
    // Generate filename with incrementing number
    std::string filename = get_next_filename(result.benchmark_name, ".json");
    
    std::ofstream json_file(filename);
    json_file << "{\n";
    json_file << "  \"benchmark_name\": \"" << result.benchmark_name << "\",\n";
    json_file << "  \"threads\": " << num_of_threads << ",\n";
    json_file << "  \"duration_ns\": " << result.duration.count() << ",\n";
    json_file << "  \"duration_ms\": " << (result.duration.count() / 1e6) << ",\n";
    json_file << "  \"messages_logged\": " << result.messages_logged << ",\n";
    json_file << "  \"messages_per_second\": " << result.messages_per_second << ",\n";
    json_file << "  \"log_file_name\": \"" << result.log_file_name << "\",\n";
    json_file << "  \"configuration\": {\n";
    json_file << "    \"queue_depth\": " << result.queue_depth << ",\n";
    json_file << "    \"batch_size\": " << result.batch_size << ",\n";
    json_file << "    \"max_logs_per_iteration\": " << result.max_logs_per_iteration << "\n";
    json_file << "  }\n";
    json_file << "}\n";
    json_file.close();
}

BenchmarkResult benchmark_logger_performance(const MR::Logger::Config& config, const std::string& benchmark_name) {
    const size_t NUM_MESSAGES = 1000000;
    
    {
        deleteIfExists(config.log_file_name);

        Logger::Logger::Factory::configure(config);
        auto logger = MR::Logger::Logger::get();
        
        // Start timing
        auto start = std::chrono::high_resolution_clock::now();
        
        // Log messages
        for (size_t i = 1; i <= NUM_MESSAGES; ++i) {
            logger->info("Benchmark message #{}", i);
        }
        
        // Stop timing
        auto end = std::chrono::high_resolution_clock::now();
        
        // Calculate duration
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        
        // Calculate messages per second
        double seconds = static_cast<double>(duration.count()) / 1e9;
        double messages_per_second = NUM_MESSAGES / seconds;
        
        // Simple stdout output - just name and duration
        std::cout << benchmark_name << ": " << (duration.count() / 1e6) << " ms" << std::endl;
        
        BenchmarkResult result{
            duration,
            NUM_MESSAGES,
            messages_per_second,
            benchmark_name,
            config.log_file_name,
            config.queue_depth,
            config.batch_size,
            config.max_logs_per_iteration
        };
        
        // Save results to JSON
        save_results_to_json(result, 1);
        
        return result;
    }
}

BenchmarkResult benchmark_logger_performance_multithreaded(const MR::Logger::Config& config, const std::string& benchmark_name, size_t num_threads, size_t messages_per_thread) {
    const size_t total_messages = num_threads * messages_per_thread;
    
    {
        deleteIfExists(config.log_file_name);

        Logger::Logger::Factory::configure(config);
        auto logger = MR::Logger::Logger::get();
        
        // Create barrier to synchronize thread start
        std::barrier sync_point(static_cast<std::ptrdiff_t>(num_threads));
        
        // Vector to store threads
        std::vector<std::thread> threads;
        threads.reserve(num_threads);
        
        // Start timing
        auto start = std::chrono::high_resolution_clock::now();
        
        // Create and start threads
        for (size_t thread_id = 0; thread_id < num_threads; ++thread_id) {
            threads.emplace_back([&sync_point, logger, messages_per_thread]() {
                // Wait for all threads to be ready
                sync_point.arrive_and_wait();
                
                // Log messages
                for (size_t i = 1; i <= messages_per_thread; ++i) {
                    logger->info("Benchmark #1");
                }
            });
        }
        
        // Wait for all threads to complete
        for (auto& thread : threads) {
            thread.join();
        }
        
        // Stop timing
        auto end = std::chrono::high_resolution_clock::now();
        
        // Calculate duration
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        
        // Calculate messages per second
        double seconds = static_cast<double>(duration.count()) / 1e9;
        double messages_per_second = total_messages / seconds;
        
        // Simple stdout output - just name and duration
        std::cout << benchmark_name << ": " << (duration.count() / 1e6) << " ms" << std::endl;
        
        BenchmarkResult result{
            duration,
            total_messages,
            messages_per_second,
            benchmark_name,
            config.log_file_name,
            config.queue_depth,
            config.batch_size,
            config.max_logs_per_iteration
        };
        
        // Save results to JSON
        save_results_to_json(result, num_threads);
        
        return result;
    }
}

}