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
The Logger behavior is controlled through the `Config` struct in `include/MR/Logger/Config.hpp`. Each option can be overriden manually when initializing the Logger singleton. All options that are unspecified will inherit the default config's values.

```cpp
#include <MR/Logger/Logger.hpp>

// Initiliaze the logger with the default config
MR::Logger::init(); 

// Initialize with a different output file name
// all other options are inherited from the default config
MR::Logger::init({.log_file_name="abc.log"});

// Get the logger singleton. Can be used anywhere in the codebase
// as long as the logger `init()` function was called once beforehand

auto log = MR::Logger::get();
log->info("Hello, World!");
```

### Thread Safe Queue
All log requests (see `include/MR/Logger/WriteRequest.hpp`) are pushed into a 
thread-safe queue (see `include/MR/Interface/ThreadSafeQueue.hpp). Write requests are dequeued by the backend loop for further processing on a worker thread. The default implementation of the `ThreadSafeQueue` is a simple wrapper class around `std::queue` with mutex locks (see `include/MR/Queue/StdQueue.hpp`). This is by far the slowest approach for an intermediary thread-safe queue yet it still beats `spdlog` in a multi-threaded environment when measuring the time to push 1m messages to the logging system.

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
The open-source library `fmt` is used for powerful, fast and type-safe formatting.

```cpp
log->info("Test 1");
log->info("Test {}", 2); // "Test 2"
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

## Platform Requirements

- **Linux**: Required for io_uring support
- **Compiler**: GCC 11+ or Clang 12+ with C++20 support
- **Kernel**: Linux 5.1+ for io_uring support
- **Architecture**: x86_64 (primary)
