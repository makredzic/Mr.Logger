# Include Util subdirectory
subdir('Util')

# Benchmark source files
benchmark_sources = benchmark_util_sources

# List of benchmark executables to build
benchmark_executables = [
  'BenchmarkDefault.cpp',
  'BenchmarkSmall.cpp', 
  'BenchmarkLarge.cpp'
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

# Custom target to run default benchmark
run_target('benchmark-default',
  command: [benchmark_exes[0]],
  depends: benchmark_exes[0]
)

# Custom target to run small benchmark
run_target('benchmark-small',
  command: [benchmark_exes[1]],
  depends: benchmark_exes[1]
)

# Custom target to run large benchmark
run_target('benchmark-large',
  command: [benchmark_exes[2]],
  depends: benchmark_exes[2]
)

# Custom target to run all benchmarks sequentially
run_target('benchmarks',
  command: [
    'sh', '-c', 
    benchmark_exes[0].full_path() + ' && ' + 
    benchmark_exes[1].full_path() + ' && ' + 
    benchmark_exes[2].full_path()
  ],
  depends: benchmark_exes
)