# Project Overview

MrLogger (pronounced *Mister Logger*) is a high-performance C++20 logging library built as part of a Master's thesis research project. It utilizes modern C++20 features (mainly coroutines) as well as io_uring for asynchronous I/O operations, comparing performance against established libraries like spdlog.


## Dependencies
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

### Platform Requirements
- **Linux-only**: Requires io_uring support (Linux 5.1+)
- **C++20 Standard**: Uses coroutines, concepts, ranges

### Testing Strategy
- **Integration Tests**: Multi-threaded scenarios in `test/Integration/`
- **Benchmark Regression**: Performance testing through comprehensive benchmark suite
- **JSON Output**: Benchmark results in structured format for analysis


## Usage

### Configuration System

The Logger behavior is controlled through the `Config` struct in `include/MR/Logger/Config.hpp`. Configuration follows a flexible inheritance model where:
- Unspecified options inherit from the default configuration
- Specified options override defaults
- Related parameters auto-scale intelligently when partially configured

#### Basic Usage

```cpp
#include <MR/Logger/Logger.hpp>

// Initialize with default config
MR::Logger::init();

// Initialize with custom file name (other options use defaults)
MR::Logger::init({.log_file_name="abc.log"});

// Get the logger singleton (call anywhere after init)
auto log = MR::Logger::get();
log->info("Hello, World!");
```

#### Batching Parameters & Auto-Scaling

The logger has three key batching parameters that work together:
- **`batch_size`**: Number of writes to batch before calling `io_uring_submit()` (default: 32)
- **`queue_depth`**: io_uring queue depth for simultaneous I/O operations (default: 512)
- **`coalesce_size`**: Target maximum messages per buffer (default: 32)

**Note**: `coalesce_size` is a target maximum. Buffers may contain fewer messages if:
- The staging buffer (16KB) reaches 90% capacity with large messages
- The queue is drained before reaching the target
- A message is too large to fit in the remaining buffer space

**Auto-Scaling Behavior**: When you specify only `batch_size`, the other parameters automatically calculate optimal values:
- `queue_depth` = 16 × `batch_size` (provides good I/O pipeline depth)
- `coalesce_size` = `batch_size` (matches batching for optimal message packing)

```cpp
// Simple: specify only batch_size, others auto-scale
MR::Logger::init({.batch_size = 64});
// Result: batch_size=64, queue_depth=1024, coalesce_size=64

// Advanced: manually override all parameters
MR::Logger::init({
    .batch_size = 32,
    .queue_depth = 512,
    .coalesce_size = 32
});
```

**Performance Guidelines**:
- **Low latency**: `batch_size = 16-32` (faster individual message processing)
- **Balanced** (default): `batch_size = 32` (good throughput with low latency)
- **High throughput**: `batch_size = 64-128` (maximum batching efficiency)

#### Inspecting Configuration

You can retrieve the final merged configuration at runtime:

```cpp
MR::Logger::init({.batch_size = 48});

auto config = MR::Logger::getConfig();
// config.batch_size = 48
// config.queue_depth = 768 (auto-scaled: 48 * 16)
// config.coalesce_size = 48 (auto-scaled to match batch_size)
```

#### Default Configuration Values

| Parameter | Default Value | Description |
|-----------|---------------|-------------|
| `log_file_name` | `"output.log"` | Output log file path |
| `max_log_size_bytes` | `5 MB` | File rotation threshold |
| `batch_size` | `32` | Write batching size |
| `queue_depth` | `512` | io_uring queue depth |
| `coalesce_size` | `32` | Message coalescing size |
| `shutdown_timeout_seconds` | `3` | Worker shutdown timeout |

### Thread Safe Queue
All log requests (see `include/MR/Logger/WriteRequest.hpp`) are pushed into a 
thread-safe queue (see `include/MR/Interface/ThreadSafeQueue.hpp`). Write requests are dequeued by the backend loop for further processing on a worker thread. The default implementation of the `ThreadSafeQueue` is a simple wrapper class around `std::queue` with mutex locks (see `include/MR/Queue/StdQueue.hpp`). This is by far the slowest approach for an intermediary thread-safe queue yet it still beats `spdlog` in a multi-threaded environment when measuring the time to push 1m messages to the logging system.

A custom implementation of a thread-safe queue can be provided when initiaiting the Logger by implementing the `ThreadSafeQueue` interface:
```cpp
template <typename T>
struct CustomQueue : public MR::Interface::ThreadSafeQueue<T>{
    inline void push(const T&) override { /*...*/}
    void push(T&&) override { /*...*/ }
    std::optional<T> tryPop() override { /*...*/ }
    std::optional<T> pop() override { /*...*/ }
    bool empty() const override { /*...*/ }
    size_t size() const override { /*...*/ }
    void shutdown() override { /*...*/ }
};

// Init the logger with your custom queue implementation
MR::Logger::init({
  ._queue = std::make_shared<CustomQueue<MR::Logger::WriteRequest>>()}
);
```

### Formatting
The open-source library [fmt](https://github.com/fmtlib/fmt) is used for powerful, fast and type-safe formatting. Anything `fmt::format` supports, MrLogger supports too!

```cpp
log->info("Test 1");
log->info("Test {} + {} is {}?", 2, 3, 1000); // "Test 2 + 3 is 1000?
```

To easily log a custom struct, simply register a transformation function that shows the string representation of your struct.
```cpp
#include <MR/Logger/Logger.hpp> // <-- also defines the MRLOGGER_TO_STRING macro

// OPTION A //
struct Point { 
  int a; 
  int b; 
};

// Pass a "to string" transformation function
std::string toStr(const Point& pt) {
  return std::string{
    "a = " + std::to_string(pt.a) + ", b = " + std::to_string(pt.b)
  }; 
}

// Register the toStr function
MRLOGGER_TO_STRING(Point, toStr)

// OPTION B //
struct Point { 
  int a; 
  int b; 

  // Create a to_string const member function
  std::string to_string() const {
    return std::string{"a = " + std::to_string(a) + ", b = " + std::to_string(b)};
  }
};

// Simply register the type
MRLOGGER_TO_STRING(Point)

// Regardless of the option (A or B)
// this now works
Point pt{6,9};
log->info("My point: {}", pt); // My point: a = 6, b = 9
```


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

## Using MRLogger as a Library

MRLogger builds as a static library (`libmrlogger.a`) and can be integrated into other projects as a Meson subproject. The build produces:
- **Static library**: `libmrlogger.a` - All logger functionality
- **Tests and Benchmarks**: Link against the library

### Integration via Wrap File (Recommended)

Create `subprojects/mrlogger.wrap` in your project:

```ini
[wrap-git]
url = https://github.com/makredzic/Mr.Logger
revision = main
depth = 1

[provide]
mrlogger = mrlogger_dep
```

In your `meson.build`:

```meson
mrlogger_dep = dependency('mrlogger', fallback: ['mrlogger', 'mrlogger_dep'])

executable('myapp', 'main.cpp', dependencies: mrlogger_dep)
```

### Integration via Git Submodule

```bash
git submodule add https://github.com/makredzic/Mr.Logger subprojects/mrlogger
```

In your `meson.build`:

```meson
mrlogger_dep = dependency('mrlogger', fallback: ['mrlogger', 'mrlogger_dep'])
executable('myapp', 'main.cpp', dependencies: mrlogger_dep)
```

The dependency includes the static library, headers, and transitive dependencies (fmt, liburing, threads) - all handled automatically by Meson.

## Platform Requirements

- **Linux**: Required for io_uring support
- **Compiler**: GCC 11+ or Clang 12+ with C++20 support
- **Kernel**: Linux 5.1+ for io_uring support
- **Architecture**: x86_64 (primary)
