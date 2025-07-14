#include "Benchmark.hpp"
#include "BenchConfigs.hpp"

int main() {
    auto config = MR::Benchmarks::BenchConfigs::get_default_config(1);
    config.name = "SingleThread-DefaultConfig";
    
    MR::Benchmarks::run_benchmark(config);
    
    return 0;
}