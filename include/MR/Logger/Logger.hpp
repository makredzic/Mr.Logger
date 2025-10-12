#pragma once

#include <fmt/format.h>
#include <fmt/compile.h>

#include <MR/Logger/SeverityLevel.hpp>
#include <MR/Logger/Config.hpp>
#include <MR/Memory/BufferPool.hpp>
#include <MR/Coroutine/WriteTask.hpp>

#include <string>

#include <MR/Logger/WriteRequest.hpp>
#include <MR/Queue/StdQueue.hpp>

#include <MR/IO/WriteOnlyFile.hpp>
#include <MR/IO/IOUring.hpp>
#include <MR/IO/FileRotater.hpp>
#include <MR/IO/WritePreparer.hpp>

#include <MR/Interface/ThreadSafeQueue.hpp>
#include <memory>
#include <thread>
#include <mutex>

// The dispatcher: selects which macro to call based on arg count.
#define GET_MACRO(_1, _2, NAME, ...) NAME

// The implementation for the 1-argument version (uses .to_string() member).
#define MRLOGGER_TO_STRING_1(type) \
  template<> \
  struct fmt::formatter<type> { \
    constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) { \
      return ctx.end(); \
    } \
    template<typename FormatContext> \
    auto format(const type& obj, FormatContext& ctx) const -> decltype(ctx.out()) { \
      return fmt::format_to(ctx.out(), "{}", obj.to_string()); \
    } \
  };

// The implementation for the 2-argument version (uses a separate function/lambda).
#define MRLOGGER_TO_STRING_2(type, lambda) \
  template<> \
  struct fmt::formatter<type> { \
    constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) { \
      return ctx.end(); \
    } \
    template<typename FormatContext> \
    auto format(const type& obj, FormatContext& ctx) const -> decltype(ctx.out()) { \
      return fmt::format_to(ctx.out(), "{}", lambda(obj)); \
    } \
  };

#define MRLOGGER_TO_STRING(...) GET_MACRO(__VA_ARGS__, MRLOGGER_TO_STRING_2, MRLOGGER_TO_STRING_1)(__VA_ARGS__)




namespace MR::Logger {
  using namespace MR::Memory;

  // Forward declaration for test class
  namespace Test { class LoggerIntegrationTest; }

  class Logger {
    private:

      inline static const Config default_config_{
        .internal_error_handler = default_error_handler,
        .log_file_name = "output.log",
        .max_log_size_bytes = 5 * 1024 * 1024,
        .batch_size = 32u,
        .queue_depth = 512u,
        .small_buffer_pool_size = 512u,
        .medium_buffer_pool_size = 256u,
        .large_buffer_pool_size = 128u,
        .small_buffer_size = 1024u,
        .medium_buffer_size = 4096u,
        .large_buffer_size = 16384u,
        .shutdown_timeout_seconds = 3u,
        ._queue = std::make_shared<Queue::StdQueue<WriteRequest>>(),
        .coalesce_size = 32u,
      };


      Config config_;
      uint16_t max_logs_per_iteration_;
      IO::WriteOnlyFile file_;
      IO::IOUring ring_;
      std::shared_ptr<Interface::ThreadSafeQueue<WriteRequest>> queue_ = nullptr;
      BufferPool buffer_pool_;
      IO::FileRotater file_rotater_;

      std::jthread worker_;

      // Flush synchronization
      std::mutex flush_mutex_;
      std::condition_variable flush_cv_;
      std::atomic<size_t> active_task_count_{0};

      // Private constructor only for the Factory class
      Logger(const Config& = default_config_);
      friend class Factory;
      
      Config mergeWithDefault(const Config& user_config);
      void eventLoop(std::stop_token);
      Coroutine::WriteTask submitWrite(std::unique_ptr<Memory::Buffer> buffer);
      void reportError(const char* location, const std::string& what) const noexcept;

      template<typename T>
      inline void write(SEVERITY_LEVEL severity, T&& data) noexcept {
        try {
          WriteRequest req{
            .level = severity,
            .data = std::forward<T>(data),
            .threadId = std::this_thread::get_id(),
            .timestamp = std::chrono::system_clock::now(),
            .sequence_number = 0  // Will be set by StdQueue::push if LOGGER_TEST_SEQUENCE_TRACKING is defined
          };
          queue_->push(std::move(req));
        } catch (const std::exception& e) {
          reportError("write to queue", e.what());
        } catch (...) {
          reportError("write to queue", "Unknown exception");
        }
      }

    public:

      template<typename... Args>
      inline void info(fmt::format_string<Args...> fmt_str, Args&&... args) noexcept {
        write(SEVERITY_LEVEL::INFO, fmt::format(fmt_str, std::forward<Args>(args)...));
      }

      template<typename T>
      inline void info(T&& str) noexcept
        requires std::is_convertible_v<std::remove_cvref_t<T>, std::string> {
        write(SEVERITY_LEVEL::INFO, std::forward<T>(str));
      }

      template<typename... Args>
      inline void warn(fmt::format_string<Args...> fmt_str, Args&&... args) noexcept {
        write(SEVERITY_LEVEL::WARN, fmt::format(fmt_str, std::forward<Args>(args)...));
      }

      template<typename T>
      inline void warn(T&& str) noexcept
        requires std::is_convertible_v<std::remove_cvref_t<T>, std::string> {
        write(SEVERITY_LEVEL::WARN, std::forward<T>(str));
      }

      template<typename... Args>
      inline void error(fmt::format_string<Args...> fmt_str, Args&&... args) noexcept {
        write(SEVERITY_LEVEL::ERROR, fmt::format(fmt_str, std::forward<Args>(args)...));
      }

      template<typename T>
      inline void error(T&& str) noexcept
        requires std::is_convertible_v<std::remove_cvref_t<T>, std::string> {
        write(SEVERITY_LEVEL::ERROR, std::forward<T>(str));
      }


      ~Logger();
      
      Logger(const Logger&) = delete;
      Logger(Logger&&) = delete;
      Logger& operator=(Logger&&) = delete;
      Logger& operator=(const Logger&) = delete;

      void warn(std::string&& str);
      void warn(const std::string& str);
      void error(std::string&& str);
      void error(const std::string& str);

      inline static const Config& defaultConfig() { return default_config_; }
      inline uint16_t getMaxLogsPerIteration() const { return max_logs_per_iteration_; }

      // Flush all currently queued log messages to disk
      void flush();

    private:
      // Factory class for singleton access
      class Factory {
        friend class Logger;

        static std::shared_ptr<Logger> instance_;
        static std::mutex mutex_;

        static std::shared_ptr<Logger> _get();
        static void init(Config&& config);
        static void _reset();
      };

    public:
      static void init(Config&& config = {});
      static void init(const Config& config);
      static std::shared_ptr<Logger> get() { return Factory::_get(); }

      /*
        Should never be called. Destroys the shared_ptr instance but does not guarantee logger shutdown due to ref counting.
        Used only for internal testing.
       */
      static void _reset();
  };

  // Namespace-level convenience functions
  void init(Config&& config = {});
  void init(const Config& config);
  std::shared_ptr<Logger> get();
}