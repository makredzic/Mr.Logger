#include "Benchmark.hpp"
#include "BenchConfigs.hpp"

int main() {
    auto config = MR::Benchmarks::BenchConfigs::get_large_config(1);
    config.name = "Large_SingleThread";
    
    MR::Benchmarks::run_benchmark(config);
    
    return 0;
}