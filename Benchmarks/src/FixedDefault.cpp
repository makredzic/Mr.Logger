#include "Benchmark.hpp"
#include "BenchConfigs.hpp"

int main() {
    auto config = MR::Benchmarks::BenchConfigs::get_fixed_default_config(1);
    config.name = "Fixed_Default_SingleThread";

    MR::Benchmarks::run_benchmark(config);

    return 0;
}
