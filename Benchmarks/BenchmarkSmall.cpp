#include <Util/BenchmarkUtils.hpp>
#include <Util/BenchmarkConfigs.hpp>

int main() {
    auto config = MR::Benchmarks::BenchmarkConfigs::get_small_config();
    auto result = MR::Benchmarks::benchmark_logger_performance(config, "BenchmarkSmall");
    
    return 0;
}