#pragma once

#include <MR/Interface/ThreadSafeQueue.hpp>
#include <MR/Logger/WriteRequest.hpp>
#include <cstdint>
#include <memory>
#include <functional>

namespace MR::Logger {

  using error_handler_t = std::function<void(const std::string&)>;
  inline void default_error_handler(const std::string& msg) noexcept {
    const char* prefix = "[MR::Logger ERROR] ";
    write(STDERR_FILENO, prefix, 19);
    write(STDERR_FILENO, msg.c_str(), msg.size());
    write(STDERR_FILENO, "\n", 1);
  }

  struct Config {

    // The handler for all MrLogger internal errors (hopefully none :))
    error_handler_t internal_error_handler = nullptr;

    // All severities always end up here
    std::string log_file_name;

    // Log files are rotated automatically. New log file will be used
    // when this size is reached. 
    size_t max_log_size_bytes;

    // Number of log messages to batch before calling io_uring_submit()
    // This is the primary tuning parameter - other batching parameters are auto-calculated from this.
    //
    // Auto-scaling behavior (when only batch_size is specified):
    //   - queue_depth = 16 × batch_size (provides good I/O pipeline depth)
    //   - coalesce_size = batch_size (matches batching for optimal message packing)
    //
    // Typical values: 16-64 (balanced), 32 (default), 128+ (high throughput)
    // Must be <= queue_depth
    uint16_t batch_size;

    // io_uring queue depth - maximum number of simultaneous I/O operations
    //
    // Auto-calculated as 16 × batch_size if not specified by user.
    // Recommended: 8-16x batch_size for optimal pipeline depth
    // Must be >= batch_size
    uint16_t queue_depth;

    // Buffer pool configuration
    uint16_t small_buffer_pool_size;
    uint16_t medium_buffer_pool_size;
    uint16_t large_buffer_pool_size;
    uint16_t small_buffer_size;
    uint16_t medium_buffer_size;
    uint16_t large_buffer_size;

    // Timeout in seconds for worker thread shutdown during logger destruction.
    // If the worker thread does not finish within this time, the destructor
    // will proceed anyway to prevent hanging.
    uint16_t shutdown_timeout_seconds;

    // For experimentation, different implementations of a ThreadSafeQueue can be used, e.g.
    // 1) StdQueue - my mutex/lock based std::queue wrapper
    // 2) moodycamel::ConcurrentQueue - lock free multi producer - multi consumer queue
    // 3) ???
    std::shared_ptr<Interface::ThreadSafeQueue<WriteRequest>> _queue;

    // Number of log messages to coalesce into a single buffer/write operation
    // This controls message-level packing before submission to io_uring.
    //
    // Auto-calculated as batch_size if not specified by user.
    // Recommended: match or be close to batch_size for optimal performance
    //
    // Higher values = fewer write operations, better throughput
    // Lower values = lower latency per message
    // 0 = disable coalescing (format and write each message individually)
    uint16_t coalesce_size;

  };
}