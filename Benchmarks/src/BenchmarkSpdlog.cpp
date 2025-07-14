#include "Benchmark.hpp"
#include "BenchConfigs.hpp"

int main() {
    auto config = MR::Benchmarks::BenchConfigs::get_spdlog_config(1);
    config.name = "BenchmarkSpdlog";
    
    MR::Benchmarks::run_benchmark(config);
    
    return 0;
}