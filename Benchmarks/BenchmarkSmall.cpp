#include <Util/BenchmarkUtils.hpp>
#include <Util/BenchmarkConfigs.hpp>
#include <iostream>

int main() {
    std::cout << "=== Small Buffer Configuration Benchmark ===" << std::endl;
    
    auto config = MR::Benchmarks::BenchmarkConfigs::get_small_config();
    auto result = MR::Benchmarks::benchmark_logger_performance(config);
    
    std::cout << "Benchmark completed successfully!" << std::endl;
    
    return 0;
}