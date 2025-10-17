#include "Benchmark.hpp"
#include "BenchConfigs.hpp"

int main() {
    auto config = MR::Benchmarks::BenchConfigs::get_no_batch_config(1);
    config.name = "NoBatch_SingleThread";

    MR::Benchmarks::run_benchmark(config);

    return 0;
}
