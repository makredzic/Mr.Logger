## Project Overview

MrLogger (pronounced *Mister Logger*) is a high-performance C++20 logging library built as part of a Master's thesis research project. It utilizes modern C++20 features (mainly coroutines) as well as io_uring for asynchronous I/O operations, comparing performance against established libraries like spdlog.


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

# Run main executable (placeholder dummy example)
./build/main

# Clean and reconfigure
rm -rf build && meson setup build
```

### Testing
The option `-Dsequence_tracking=true` is only for running an extra test. It enables the `LOGGER_TEST_SEQUENCE_TRACKING` preprocessor flag which adds additional code into the logger's implementation that adds a sequence number to each message. This is used for validating that the order in which messages are submitted to the intermediary queue
by multiple threads concurrently is maintained when it finally reaches the log file. 

```bash
# Build project with sequence tracking for extra ordering tests (recommended when testing)
meson setup build -Dsequence_tracking=true

# Compile 
ninja -C build

# Run all tests
meson test -C build
```

### Benchmarking
```bash
# Run 1 quick iteration of all benchmarks (vs spdlog)
ninja benchmarks # in the build dir

# Automated benchmark suite with analysis and plotting
# 3 - number of times EACH test will be run to get the min,max,median and avg time
python3 benchmark_runner.py 3
```

## Platform Requirements

- **Linux**: Required for io_uring support
- **Compiler**: GCC 11+ or Clang 12+ with C++20 support
- **Kernel**: Linux 5.1+ for io_uring support
- **Architecture**: x86_64 (primary)
