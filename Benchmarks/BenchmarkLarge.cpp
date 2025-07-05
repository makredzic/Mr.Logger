#include <Util/BenchmarkUtils.hpp>
#include <Util/BenchmarkConfigs.hpp>
#include <iostream>

int main() {
    std::cout << "=== Large Buffer Configuration Benchmark ===" << std::endl;
    
    auto config = MR::Benchmarks::BenchmarkConfigs::get_large_config();
    auto result = MR::Benchmarks::benchmark_logger_performance(config);
    
    std::cout << "Benchmark completed successfully!" << std::endl;
    
    return 0;
}