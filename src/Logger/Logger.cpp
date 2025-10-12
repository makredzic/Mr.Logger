#include <MR/Logger/Config.hpp>
#include <MR/Logger/WriteRequest.hpp>
#include <MR/Logger/SeverityLevel.hpp>
#include <MR/Interface/ThreadSafeQueue.hpp>
#include <MR/Logger/Logger.hpp>
#include <MR/Memory/BufferPool.hpp>
#include <MR/Coroutine/WriteTask.hpp>


#include <iostream>
#include <stdexcept>
#include <stop_token>
#include <string>
#include <thread>
#include <future>
#include <cmath>

#include <fmt/core.h>
#include <fmt/std.h>
#include <fmt/chrono.h>


namespace MR::Logger {

Config Logger::mergeWithDefault(const Config& user_config) {
  return Config{
  .internal_error_handler = user_config.internal_error_handler == nullptr
    ? default_config_.internal_error_handler
    : user_config.internal_error_handler,

  .log_file_name = user_config.log_file_name.empty()
      ? default_config_.log_file_name
      : user_config.log_file_name,

  .max_log_size_bytes = user_config.max_log_size_bytes == 0
    ? default_config_.max_log_size_bytes
    : user_config.max_log_size_bytes,

  .batch_size = user_config.batch_size == 0
    ? default_config_.batch_size
    : user_config.batch_size,

  .queue_depth = user_config.queue_depth == 0
    ? default_config_.queue_depth
    : user_config.queue_depth,

  .small_buffer_pool_size = user_config.small_buffer_pool_size == 0
    ? default_config_.small_buffer_pool_size
    : user_config.small_buffer_pool_size,

  .medium_buffer_pool_size = user_config.medium_buffer_pool_size == 0
    ? default_config_.medium_buffer_pool_size
    : user_config.medium_buffer_pool_size,

  .large_buffer_pool_size = user_config.large_buffer_pool_size == 0
    ? default_config_.large_buffer_pool_size
    : user_config.large_buffer_pool_size,

  .small_buffer_size = user_config.small_buffer_size == 0
    ? default_config_.small_buffer_size
    : user_config.small_buffer_size,

  .medium_buffer_size = user_config.medium_buffer_size == 0
    ? default_config_.medium_buffer_size
    : user_config.medium_buffer_size,

  .large_buffer_size = user_config.large_buffer_size == 0
    ? default_config_.large_buffer_size
    : user_config.large_buffer_size,

  .shutdown_timeout_seconds = user_config.shutdown_timeout_seconds == 0
    ? default_config_.shutdown_timeout_seconds
    : user_config.shutdown_timeout_seconds,

  ._queue = user_config._queue == nullptr
    ? default_config_._queue
    : user_config._queue
  };
}

Logger::Logger(const Config& config) :
  config_(mergeWithDefault(config)),
  max_logs_per_iteration_(std::min<uint16_t>(
    config_.queue_depth / 2,
    std::max<uint16_t>(
      config_.batch_size * 2,
      static_cast<uint16_t>(config_.batch_size * std::sqrt(static_cast<double>(config_.queue_depth) / config_.batch_size))
    )
  )),
  file_{config_.log_file_name},
  ring_{config_.queue_depth},
  queue_{config_._queue},
  file_rotater_{config_.log_file_name, config_.max_log_size_bytes},
  worker_{
  [this](std::stop_token st){
      try {
        eventLoop(st);
      } catch (const std::exception& e) {
        std::cerr << "[MrLogger] Exception caught from main eventLoop. Shutting down logger. Exception = " << e.what() << "\n";
      } catch (...) {
        std::cerr << "[MrLogger] Unknown exception caught from main eventLoop. Shutting down logger.\n";
      }

      queue_->shutdown();
    }
  } {

    if (config_.batch_size > config_.queue_depth) {
      throw std::invalid_argument{"batch_size cannot exceed queue_depth"};
    }

    // Warn about suboptimal configurations
    if (config_.batch_size > config_.queue_depth / 2) {
      reportError("constructor",
        "Warning: batch_size (" + std::to_string(config_.batch_size) +
        ") is more than half of queue_depth (" + std::to_string(config_.queue_depth) +
        "). This may result in inefficient CQE processing. Consider reducing batch_size or increasing queue_depth.");
    }

    // Ensure calculated max_logs_per_iteration allows at least 2 batches
    if (max_logs_per_iteration_ < config_.batch_size * 2) {
      reportError("constructor",
        "Warning: Calculated max_logs_per_iteration (" + std::to_string(max_logs_per_iteration_) +
        ") is less than 2x batch_size (" + std::to_string(config_.batch_size * 2) +
        "). This may cause excessive syscall overhead. Consider adjusting queue_depth.");
    }

  }

  void Logger::eventLoop(std::stop_token st) {

    // Required to hold the state of the coroutines while they
    // are suspended and not finished
    std::vector<Coroutine::WriteTask> active_tasks;
    active_tasks.reserve(max_logs_per_iteration_*2);

    size_t pending_writes = 0;

    while(!st.stop_requested() || !queue_->empty() || !active_tasks.empty()) {

      // Check if io_uring has failed - if so, drain queue and exit
      if (!ring_.isOperational()) {
        reportError("eventLoop", "io_uring marked as failed. Draining queue and shutting down.");

        // Drain remaining queue items without processing
        size_t dropped = 0;
        while (queue_->tryPop()) {
          dropped++;
        }

        if (dropped > 0) {
          reportError("eventLoop", "Dropped " + std::to_string(dropped) + " log messages due to io_uring failure.");
        }

        break;  // Exit event loop
      }

      size_t processed_this_iteration = 0;

      // NON BLOCKING REMOVAL AS LONG AS THERE ARE ELEMENTS
      while (auto request = queue_->tryPop()) {
        // Don't process new requests if ring has failed
        if (!ring_.isOperational()) {
          reportError("eventLoop", "Skipping request because io_uring is not operational");
          break;
        }

        try {
          active_tasks.push_back(processRequest(std::move(request.value())));
          active_task_count_.fetch_add(1, std::memory_order_release);

          pending_writes++;
          processed_this_iteration++;

          if (pending_writes >= config_.batch_size) {
            if (!ring_.submitPendingSQEs()) {
              reportError("eventLoop:submit", "Failed to submit batch. io_uring may be degraded.");
            }
            pending_writes = 0;
          }

          // Prevents too many requests from causing this loop to stall
          // and processCompletions not being called below
          if (processed_this_iteration >= max_logs_per_iteration_) break;

        } catch (const std::exception& e) {
          reportError("eventLoop:processing", e.what());
        } catch (...) {
          reportError("eventLoop:processing", "Unknown exception");
        }
      }


      // Submit any remaining requests
      if (pending_writes > 0) {
        if (!ring_.submitPendingSQEs()) {
          reportError("eventLoop:submit", "Failed to submit remaining writes.");
        }
        pending_writes = 0;
      }

      // Process CQEs (maybe dispatch to thread pool)
      ring_.processCompletions();

      // Clean up completed tasks and check for exceptions
      active_tasks.erase(
          std::remove_if(active_tasks.begin(), active_tasks.end(),
              [this](const Coroutine::WriteTask& task) {
                if (!task.done()) return false;

                // Check if task completed with exception
                if (task.has_exception()) {
                  try {
                    task.rethrow_if_exception();
                  } catch (const std::exception& e) {
                    reportError("coroutine", e.what());
                  } catch (...) {
                    reportError("coroutine", "Unknown exception in completed task");
                  }
                }

                // Decrement active task count and notify flush if this was the last task
                if (active_task_count_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
                  // This was the last active task - notify any waiting flush
                  flush_cv_.notify_one();
                }

                return true;
              }),
          active_tasks.end()
      );

      // Smart waiting strategy
      if (!st.stop_requested()) {
        if (queue_->empty() && !active_tasks.empty()) {
          // We have outstanding I/O but no new work - wait for completions
          ring_.waitForCompletion(std::chrono::microseconds(1000));
        } else if (queue_->empty() && active_tasks.empty()) {
          // Truly idle - brief sleep
          std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
        // else: work available, don't sleep at all
      }
    }
  }

  Coroutine::WriteTask Logger::processRequest(WriteRequest&& request) {

    try {
      // Check if file rotation is needed
      if (file_rotater_.shouldRotate()) {
        file_rotater_.rotate();
        file_.reopen(file_rotater_.getCurrentFilename());
      }

      // Estimate required buffer size (with some padding for safety)
      size_t estimated_size = request.data.size() + 256; // Extra for timestamp, level, thread ID

      // Acquire buffer from pool
      auto buffer = buffer_pool_.acquire(estimated_size);

      // Format directly into buffer
      size_t actual_size = formatTo(std::move(request), buffer->as_char(), buffer->capacity);
      buffer->size = actual_size;

      // Submit write and wait for completion
      int bytes_written = co_await ring_.write(file_, buffer->data, buffer->size);

      // THIS IS RESUMED AFTER THE KERNEL CONSUMES THE CQE
      // Todo: add possible thread pool task
      buffer_pool_.release(std::move(buffer));
      // Handle write result
      if (bytes_written < 0) {
        reportError("processRequest:write", "io_uring write failed with error code: " + std::to_string(bytes_written));
      } else {
        // Update file rotater with bytes written
        file_rotater_.updateCurrentSize(bytes_written);
      }

    } catch (const std::exception& e) {
      reportError("processRequest", e.what());
    } catch (...) {
      reportError("processRequest", "Unknown exception");
    }
  }

  size_t Logger::formatTo(WriteRequest&& writeRequest, char* buffer, size_t capacity) {

#ifdef LOGGER_TEST_SEQUENCE_TRACKING
    // Debug: This should be executed when testing
    auto result = fmt::format_to_n(
      buffer, capacity - 1,  // Reserve space for null terminator
      "[{}] [{}] [Thread: {}] [Seq: {}]: {}\n",
      writeRequest.timestamp,
      sevLvlToStr(writeRequest.level),
      writeRequest.threadId,
      writeRequest.sequence_number,
      std::move(writeRequest.data)
    );
#else
    // Debug: This should NOT be executed when testing
    auto result = fmt::format_to_n(
      buffer, capacity - 1,  // Reserve space for null terminator
      "[{}] [{}] [Thread: {}]: {}\n",
      writeRequest.timestamp,
      sevLvlToStr(writeRequest.level),
      writeRequest.threadId,
      std::move(writeRequest.data)
    );
#endif
    
    // Null terminate (myb not necessary for io_uring??)
    if (result.size < capacity) {
      buffer[result.size] = '\0';
    }
    
    return result.size;
  }


  Logger::~Logger() {

    queue_->shutdown();

    if (worker_.joinable()) {
      worker_.request_stop();

      auto join_future = std::async(std::launch::async, [this]() {
        worker_.join();
      });

      const auto timeout = std::chrono::seconds(config_.shutdown_timeout_seconds);
      if (join_future.wait_for(timeout) == std::future_status::timeout) {
        reportError("destructor", "Worker thread did not exit within " +
                   std::to_string(config_.shutdown_timeout_seconds) +
                   " seconds. Resources may leak.");
      }
    }
  }

  // Factory static member definitions
  std::shared_ptr<Logger> Logger::Factory::instance_ = nullptr;
  std::mutex Logger::Factory::mutex_;

  void Logger::Factory::init(Config&& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!instance_) {
      instance_ = std::shared_ptr<Logger>(new Logger(config));
    }
  }

  std::shared_ptr<Logger> Logger::Factory::_get() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!instance_) throw std::runtime_error{"MR::Logger instance not created. MR::Logger::init() must be called before any call to MR::Logger::get()."};
    return instance_;
  }

  void Logger::Factory::_reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    instance_.reset();
  }

  void Logger::init(Config&& config) {
    Factory::init(std::move(config));
  }
  void Logger::init(const Config& config) {
    Factory::init(Config{config});
  }
  void Logger::_reset() {
    Factory::_reset();
  }

  void Logger::reportError(const char* location, const std::string& what) const noexcept {
    try {
      std::string msg = std::string("[") + location + "] " + what;
      config_.internal_error_handler(msg);
    } catch (...) { // If error handler itself throws, fall back to raw stderr
      const char* critical = "[MR::Logger CRITICAL] Error handler threw\n";
      ::write(STDERR_FILENO, critical, strlen(critical));
    }
  }

  void Logger::flush() {
    std::unique_lock<std::mutex> lock(flush_mutex_);

    // Wait until queue is empty AND all active tasks are done
    flush_cv_.wait(lock, [this]() {
      return queue_->size() == 0 && active_task_count_.load(std::memory_order_acquire) == 0;
    });
  }

  // Namespace-level convenience functions
  void init(Config&& config) {
    Logger::init(std::move(config));
  }

  void init(const Config& config) {
    Logger::init(config);
  }

  std::shared_ptr<Logger> get() {
    return Logger::get();
  }
};