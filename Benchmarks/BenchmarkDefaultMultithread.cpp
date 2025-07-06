#include <Util/BenchmarkUtils.hpp>
#include <Util/BenchmarkConfigs.hpp>

int main() {
    auto config = MR::Benchmarks::BenchmarkConfigs::get_default_config();
    auto result = MR::Benchmarks::benchmark_logger_performance_multithreaded(config, "Multithread-DefaultConfig");
    
    return 0;
}