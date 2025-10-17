#include "Benchmark.hpp"
#include "BenchConfigs.hpp"

int main() {
    auto config = MR::Benchmarks::BenchConfigs::get_default_config(10);
    config.name = "Default_MultiThread";
    
    MR::Benchmarks::run_benchmark(config);
    
    return 0;
}