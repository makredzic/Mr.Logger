#include "Benchmark.hpp"
#include "BenchConfigs.hpp"

int main() {
    auto config = MR::Benchmarks::BenchConfigs::get_spdlog_config(10);
    config.name = "Spdlog_MultiThread";
    
    MR::Benchmarks::run_benchmark(config);
    
    return 0;
}