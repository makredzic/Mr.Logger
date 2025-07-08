#include <Util/SpdlogBenchmarkUtils.hpp>
#include <Util/SpdlogBenchmarkConfigs.hpp>

int main() {
    auto config = MR::Benchmarks::SpdlogBenchmarkConfigs::get_multithread_config();
    auto result = MR::Benchmarks::benchmark_spdlog_performance_multithreaded(config, "BenchmarkSpdlogMultithread");
    
    return 0;
}