## Project Overview

MRLogger is a high-performance C++20 logging library built as part of a Master's thesis research project. It utilizes modern C++20 features (mainly coroutines) as well as io_uring for asynchronous I/O operations, comparing performance against established libraries like spdlog.


### Dependencies
- **liburing** - Linux io_uring for async I/O (required)
- **fmt** - Fast C++ formatting library
- **GoogleTest** - Testing framework
- **C++20 compiler** - GCC 11+ or Clang 12+

## Architecture Overview

### Core Design Patterns
- **Singleton Factory Pattern**: Logger::get() provides global access to configured logger instance
- **Asynchronous I/O**: Uses io_uring coroutines for non-blocking file operations
- **Producer-Consumer**: Thread-safe queues decouple logging calls from I/O operations
- **RAII Resource Management**: Smart pointers and RAII principles throughout

### Key Components Structure
- **Logger Core** (`include/MR/Logger/`): Main logging interface and configuration
- **I/O System** (`include/MR/IO/`): io_uring integration and file abstractions  
- **Coroutine Infrastructure** (`include/MR/Coroutine/`): C++20 coroutine async write operations
- **Queue System** (`include/MR/Queue/`): Thread-safe queue implementations
- **Memory Management** (`include/MR/Memory/`): Buffer pooling and memory allocation
- **Interfaces** (`include/MR/Interface/`): Abstract base classes for pluggable components

### Thread Safety Model
The logger uses a thread-safe design with configurable thread-safe queue implementations. In its most basic form, the logger uses a blocking wrapper class around `std::queue` but the `Interface::ThreadSafeQueue` can be implemented freely for more optimized performance, replacing `StdQueue`. All public logging methods (info, warn, error) can be called concurrently from multiple threads.

### Configuration System
Logger behavior is controlled through the `Config` struct in `include/MR/Logger/Config.hpp`:
- **queue_depth**: io_uring submission queue depth
- **batch_size**: Number of operations submitted per io_uring batch
- **max_logs_per_iteration**: Limits processing to prevent starvation
- Custom file paths for different severity levels

### Platform Requirements
- **Linux-only**: Requires io_uring support (Linux 5.1+)
- **C++20 Standard**: Uses coroutines, concepts, ranges
- **Release Builds**: Optimized for performance (`buildtype=release`)


### Testing Strategy
- **Integration Tests**: Multi-threaded scenarios in `test/Integration/`
- **Benchmark Regression**: Performance testing through comprehensive benchmark suite
- **JSON Output**: Benchmark results in structured format for analysis


### Build & Compilation
```bash
# Configure build (from repository root)
meson setup build

# Compile all targets
meson compile -C build

# Run main executable
./build/main

# Clean and reconfigure
rm -rf build && meson setup build
```

### Testing
```bash
# Run all tests
meson test -C build
```

### Benchmarking
```bash
# Individual benchmark executables
./build/Benchmarks/BenchmarkDefault
./build/Benchmarks/BenchmarkSmall
./build/Benchmarks/BenchmarkLarge
./build/Benchmarks/BenchmarkDefaultMultithread
./build/Benchmarks/BenchmarkSmallMultithread  
./build/Benchmarks/BenchmarkLargeMultithread
./build/Benchmarks/BenchmarkSpdlog
./build/Benchmarks/BenchmarkSpdlogMultithread

# Automated benchmark suite with analysis
python3 benchmark_runner.py . 10
```

## Platform Requirements

- **Linux**: Required for io_uring support
- **Compiler**: GCC 11+ or Clang 12+ with C++20 support
- **Kernel**: Linux 5.1+ for io_uring support
- **Architecture**: x86_64 (primary)
