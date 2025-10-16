#include "Benchmark.hpp"
#include "BenchConfigs.hpp"

int main() {
    auto config = MR::Benchmarks::BenchConfigs::get_fixed_default_config(10);
    config.name = "Bench_Fixed_Default_Multithreaded";

    MR::Benchmarks::run_benchmark(config);

    return 0;
}
