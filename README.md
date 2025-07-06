# MRLogger - High-Performance C++ Logger

## Project Overview

**MRLogger** is a high-performance, multi-threaded C++ logging library designed for speed and efficiency. It's built as part of a Master's thesis research project comparing logging performance with other libraries like spdlog.

### Key Features
- **Asynchronous logging** with io_uring for high-performance I/O operations
- **Thread-safe** design with configurable queue implementations
- **Coroutine-based** write operations for efficient resource utilization
- **Singleton pattern** with factory for global logger access
- **Configurable batching** for optimal I/O performance
- **Multiple severity levels** (INFO, WARN, ERROR)
- **Comprehensive benchmarking** suite for performance analysis

## Architecture

### Core Components

#### 1. **Logger Core** (`include/MR/Logger/`)
- **Logger.hpp/cpp**: Main logger class with singleton factory pattern
- **Config.hpp**: Configuration structure for logger parameters
- **WriteRequest.hpp**: Request structure for log entries
- **SeverityLevel.hpp**: Severity level definitions

#### 2. **I/O System** (`include/MR/IO/`)
- **IOUring.hpp/cpp**: io_uring integration for high-performance async I/O
- **WriteOnlyFile.hpp**: File abstraction for log output

#### 3. **Coroutine System** (`include/MR/Coroutine/`)
- **WriteTask.hpp**: Coroutine implementation for async write operations

#### 4. **Queue System** (`include/MR/Queue/`)
- **StdQueue.hpp**: Thread-safe queue implementation using std::queue + mutex
- **ThreadSafeQueue.hpp**: Interface for pluggable queue implementations

#### 5. **Benchmarking** (`Benchmarks/`)
- Performance benchmarks for single and multi-threaded scenarios
- Configurable benchmark parameters for different queue sizes and batch settings
- JSON output for performance analysis

## Technology Stack

- **Language**: C++20
- **Build System**: Meson
- **Testing**: Google Test (gtest/gmock)
- **Dependencies**:
  - `liburing` (Linux io_uring for async I/O)
  - `fmt` (Fast formatting library)
  - `threads` (C++ threading support)
- **Performance**: io_uring + coroutines for minimal overhead async I/O

## Directory Structure

```
/home/mredzic/Documents/FET/Magistarski/Logger/
├── include/MR/           # Header files organized by component
│   ├── Logger/          # Core logger headers
│   ├── IO/              # I/O system headers  
│   ├── Coroutine/       # Coroutine infrastructure
│   ├── Queue/           # Queue implementations
│   └── Interface/       # Abstract interfaces
├── src/                 # Implementation files
│   ├── Logger/          # Logger implementation
│   ├── IO/IOUring/      # io_uring implementation
│   └── main.cpp         # Example/demo program
├── test/                # Test suite
│   ├── Integration/     # Integration tests
│   └── Unit/           # Unit tests (placeholder)
├── Benchmarks/          # Performance benchmarking suite
├── spdlog-research/     # spdlog library for comparison
├── subprojects/         # Meson dependencies (fmt, gtest, liburing)
└── Results/            # Benchmark results storage
```

## Build & Development Commands

### Building
```bash
# Configure build
meson setup build

# Compile
meson compile -C build

# Run main executable
./build/main
```

### Testing
```bash
# Run all tests
meson test -C build

# Run specific test suite
meson test -C build --suite integration
```

### Benchmarking
```bash
# Run benchmarks
./build/BenchmarkDefault
./build/BenchmarkDefaultMultithread
./build/BenchmarkSmall
./build/BenchmarkLarge
```

## Configuration

The logger is highly configurable through the `Config` struct:

```cpp
struct Config {
    std::string log_file_name;      // Output file path
    std::string info_file_name;     // Optional separate info file
    std::string warn_file_name;     // Optional separate warn file  
    std::string error_file_name;    // Optional separate error file
    uint16_t queue_depth;           // io_uring queue depth
    uint16_t batch_size;            // Batch size for io_uring submissions
    uint16_t max_logs_per_iteration; // Max logs processed per event loop
    std::shared_ptr<Interface::ThreadSafeQueue<WriteRequest>> _queue;
};
```

## Usage Example

```cpp
#include <MR/Logger/Logger.hpp>

int main() {
    // Get singleton logger instance
    auto logger = MR::Logger::Logger::get();
    
    // Log messages
    logger->info("Application started");
    logger->warn("This is a warning");
    logger->error("An error occurred");
    
    return 0;
}
```

## Performance Characteristics

- **Async I/O**: Uses io_uring for kernel-level asynchronous I/O operations
- **Batching**: Configurable batch sizes to optimize I/O throughput
- **Thread Safety**: Lock-free where possible, minimal contention
- **Memory Management**: Efficient buffer management with planned memory pooling
- **Coroutines**: C++20 coroutines for efficient async operations without callback hell

## Research Context

This project is part of a Master's thesis analyzing the performance of a fully async logger that utilizes C++20 Coroutines and IO_URING.

## Development Guidelines

### Code Style
- C++20 standard features encouraged
- RAII principles for resource management
- Modern C++ idioms (smart pointers, ranges, etc.)
- Namespace organization: `MR::<Component>::<Class>`

### Testing
- Comprehensive integration tests for multi-threaded scenarios
- Performance regression testing through benchmarks
- Memory leak detection (planned)

### Performance Considerations
- Minimize allocations in hot paths
- Use move semantics for string operations
- Batch I/O operations for throughput
- Consider NUMA topology for multi-threaded scenarios

## (Possible) Future Enhancements

- **Memory Pool**: Buffer pooling to reduce malloc/free overhead
- **Lock-free Queues**: Integration with moodycamel::ConcurrentQueue
- **Structured Logging**: JSON/structured format support
- **Network Sinks**: TCP/UDP log forwarding
- **Compression**: Log compression for storage efficiency
- **Metrics**: Built-in performance metrics collection

## Platform Requirements

- **Linux**: Required for io_uring support
- **Compiler**: GCC 11+ or Clang 12+ with C++20 support
- **Kernel**: Linux 5.1+ for io_uring support
- **Architecture**: x86_64 (primary)