#include "Benchmark.hpp"
#include "BenchConfigs.hpp"

int main() {
    auto config = MR::Benchmarks::BenchConfigs::get_small_config(1);
    config.name = "Bench_Small_SingleThread";
    
    MR::Benchmarks::run_benchmark(config);
    
    return 0;
}