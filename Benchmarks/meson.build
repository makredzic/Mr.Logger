# Include Util subdirectory
subdir('Util')

# Benchmark source files
benchmark_sources = benchmark_util_sources

# List of benchmark executables to build
benchmark_executables = [
  'BenchmarkDefault.cpp',
  'BenchmarkSmall.cpp', 
  'BenchmarkLarge.cpp',
  'BenchmarkDefaultMultithread.cpp',
  'BenchmarkSmallMultithread.cpp',
  'BenchmarkLargeMultithread.cpp',
]

# List of spdlog benchmark executables to build
spdlog_benchmark_executables = [
  'BenchmarkSpdlog.cpp',
  'BenchmarkSpdlogMultithread.cpp'
]

# Build each benchmark executable and store references
benchmark_exes = []
foreach exe_src : benchmark_executables
  # Extract executable name from source filename
  exe_name = exe_src.split('.')[0]
  
  exe = executable(exe_name,
    [lib_src, benchmark_sources, files(exe_src)],
    include_directories: incdir,
    dependencies: deps,
    install: false
  )
  
  # Store executable reference for custom targets
  benchmark_exes += exe
endforeach

# Build spdlog benchmark executables and store references
spdlog_benchmark_exes = []
foreach exe_src : spdlog_benchmark_executables
  # Extract executable name from source filename
  exe_name = exe_src.split('.')[0]
  
  exe = executable(exe_name,
    [spdlog_benchmark_util_sources, files(exe_src)],
    include_directories: incdir,
    dependencies: deps,
    install: false
  )
  
  # Store executable reference for custom targets
  spdlog_benchmark_exes += exe
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

# Custom target to run default multithread benchmark
run_target('bench-default-multi',
  command: [benchmark_exes[3]],
  depends: benchmark_exes[3]
)

# Custom target to run small multithread benchmark
run_target('bench-small-multi',
  command: [benchmark_exes[4]],
  depends: benchmark_exes[4]
)

# Custom target to run large multithread benchmark
run_target('bench-large-multi',
  command: [benchmark_exes[5]],
  depends: benchmark_exes[5]
)

# Custom target to run spdlog benchmark
run_target('bench-spdlog',
  command: [spdlog_benchmark_exes[0]],
  depends: spdlog_benchmark_exes[0]
)

# Custom target to run spdlog multithread benchmark
run_target('bench-spdlog-multi',
  command: [spdlog_benchmark_exes[1]],
  depends: spdlog_benchmark_exes[1]
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

# Custom target to run all spdlog benchmarks sequentially
run_target('spdlog-benchmarks',
  command: [
    'sh', '-c', 
    spdlog_benchmark_exes[0].full_path() + ' && ' + 
    spdlog_benchmark_exes[1].full_path()
  ],
  depends: spdlog_benchmark_exes
)