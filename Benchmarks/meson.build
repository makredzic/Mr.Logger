# Include directories
benchmark_incdir = include_directories('include')

# Benchmark core source files
benchmark_core_sources = files(
    'src/Benchmark.cpp'
)

# List of benchmark executables to build with new naming convention
benchmark_config = [
    ['BenchmarkDefault.cpp', 'Bench_Default_SingleThread'],
    ['BenchmarkSmall.cpp', 'Bench_Small_SingleThread'],
    ['BenchmarkLarge.cpp', 'Bench_Large_SingleThread'],
    ['BenchmarkNoBatch.cpp', 'Bench_NoBatch_SingleThread'],
    ['BenchmarkSpdlog.cpp', 'Bench_Spdlog_SingleThread'],
    ['BenchmarkDefaultMultiThreaded.cpp', 'Bench_Default_MultiThread'],
    ['BenchmarkSpdlogMultiThreaded.cpp', 'Bench_Spdlog_MultiThread'],
    ['BenchmarkFixedDefault.cpp', 'Bench_Fixed_Default_SingleThread'],
    ['BenchmarkFixedSmall.cpp', 'Bench_Fixed_Small_SingleThread'],
    ['BenchmarkFixedLarge.cpp', 'Bench_Fixed_Large_SingleThread'],
    ['BenchmarkFixedDefaultMultiThreaded.cpp', 'Bench_Fixed_Default_Multithreaded'],
]

# Build each benchmark executable and store references
benchmark_exes = []
foreach bench_info : benchmark_config
  exe_src = bench_info[0]
  exe_name = bench_info[1]
  
  exe = executable(exe_name,
    [benchmark_core_sources, files('src/' + exe_src)],
    include_directories: [incdir, benchmark_incdir],
    dependencies: deps,
    link_with: mrlogger_lib,
    install: false
  )
  
  # Store executable reference for custom targets
  benchmark_exes += exe
endforeach

# Custom target to run default benchmark
run_target('bench-default',
  command: [benchmark_exes[0]],
  depends: benchmark_exes[0]
)

# Custom target to run small benchmark
run_target('bench-small',
  command: [benchmark_exes[1]],
  depends: benchmark_exes[1]
)

# Custom target to run large benchmark
run_target('bench-large',
  command: [benchmark_exes[2]],
  depends: benchmark_exes[2]
)

# Custom target to run spdlog benchmark
run_target('bench-spdlog',
  command: [benchmark_exes[4]],
  depends: benchmark_exes[4]
)

# Custom target to run fixed-size queue benchmarks
run_target('bench-fixed-default',
  command: [benchmark_exes[7]],
  depends: benchmark_exes[7]
)

run_target('bench-fixed-small',
  command: [benchmark_exes[8]],
  depends: benchmark_exes[8]
)

run_target('bench-fixed-large',
  command: [benchmark_exes[9]],
  depends: benchmark_exes[9]
)

run_target('bench-fixed-multithreaded-default',
  command: [benchmark_exes[10]],
  depends: benchmark_exes[10]
)

# Custom target to run all benchmarks sequentially
run_target('benchmarks',
  command: [
    'sh', '-c',
    benchmark_exes[0].full_path() + ' && ' +
    benchmark_exes[1].full_path() + ' && ' +
    benchmark_exes[2].full_path() + ' && ' +
    benchmark_exes[3].full_path() + ' && ' +
    benchmark_exes[4].full_path() + ' && ' +
    benchmark_exes[5].full_path() + ' && ' +
    benchmark_exes[6].full_path() + ' && ' +
    benchmark_exes[7].full_path() + ' && ' +
    benchmark_exes[8].full_path() + ' && ' +
    benchmark_exes[9].full_path() + ' && ' +
    benchmark_exes[10].full_path()
  ],
  depends: benchmark_exes
)