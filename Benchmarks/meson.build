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
    ['BenchmarkSpdlog.cpp', 'Bench_Spdlog_SingleThread'],
    ['BenchmarkDefaultMultiThreaded.cpp', 'Bench_Default_MultiThread'],
    ['BenchmarkSpdlogMultiThreaded.cpp', 'Bench_Spdlog_MultiThread'],
]

# Build each benchmark executable and store references
benchmark_exes = []
foreach bench_info : benchmark_config
  exe_src = bench_info[0]
  exe_name = bench_info[1]
  
  exe = executable(exe_name,
    [lib_src, benchmark_core_sources, files('src/' + exe_src)],
    include_directories: [incdir, benchmark_incdir],
    dependencies: deps,
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
  command: [benchmark_exes[3]],
  depends: benchmark_exes[3]
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
    benchmark_exes[5].full_path()
  ],
  depends: benchmark_exes
)