#include "Benchmark.hpp"
#include "BenchConfigs.hpp"

int main() {
    auto config = MR::Benchmarks::BenchConfigs::get_circular_default_config(10);
    config.name = "Circular_Default_Multithreaded";

    MR::Benchmarks::run_benchmark(config);

    return 0;
}
