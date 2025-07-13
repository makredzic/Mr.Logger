#pragma once

#include <fmt/format.h>
#include <fmt/compile.h>

#include "MR/Logger/SeverityLevel.hpp"
#include <MR/Logger/Config.hpp>
#include <MR/Memory/BufferPool.hpp>
#include <MR/Coroutine/WriteTask.hpp>

#include <string>

#include <MR/Logger/WriteRequest.hpp>
#include <MR/Queue/StdQueue.hpp>

#include <MR/IO/WriteOnlyFile.hpp>
#include <MR/IO/IOUring.hpp>

#include <MR/Interface/ThreadSafeQueue.hpp>
#include <memory>
#include <thread>
#include <mutex>

// The same macro from before
#define REGISTER_MR_LOGGER_TO_STRING(type, lambda) \
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


namespace MR::Logger {
  using namespace MR::Memory;

  // Forward declaration for test class
  namespace Test { class LoggerIntegrationTest; }

  class Logger {
    private:

      // CONFIGURATION
      inline static Config default_config_{
        .log_file_name = "output.log",
        .info_file_name = "",
        .warn_file_name = "",
        .error_file_name = "",
        .queue_depth = 256u,
        .batch_size = 10u,
        .max_logs_per_iteration = 10u,
        .small_buffer_pool_size = 128u,
        .medium_buffer_pool_size = 64u,
        .large_buffer_pool_size = 32u,
        .small_buffer_size = 1024u,
        .medium_buffer_size = 4096u,
        .large_buffer_size = 16384u,
        ._queue = std::make_shared<Queue::StdQueue<WriteRequest>>(),
      };


      Config config_;
      IO::WriteOnlyFile file_;
      IO::IOUring ring_;
      std::shared_ptr<Interface::ThreadSafeQueue<WriteRequest>> queue_;
      BufferPool buffer_pool_;

      std::jthread worker_;

      // Private constructor - only accessible through Factory
      Logger(const Config& = default_config_);
      friend class Factory;
      
      Config mergeWithDefault(const Config& user_config);
      void eventLoop(std::stop_token);
      size_t formatTo(WriteRequest&& msg, char* buffer, size_t capacity);
      Coroutine::WriteTask processRequest(WriteRequest&&);

      template<typename T>
      inline void write(SEVERITY_LEVEL severity, T&& data) {
        WriteRequest req{
          .level = severity,
          .data = std::forward<T>(data),
          .threadId = std::this_thread::get_id(),
          .timestamp = std::chrono::system_clock::now()
        };
        queue_->push(std::move(req));
      }

    public:

      template<typename... Args>
      inline void info(fmt::format_string<Args...> fmt_str, Args&&... args) {
        write(SEVERITY_LEVEL::INFO, fmt::format(fmt_str, std::forward<Args>(args)...));
      }

      template<typename T>
      inline void info(T&& str) 
        requires std::is_convertible_v<std::remove_cvref_t<T>, std::string> {
        write(SEVERITY_LEVEL::INFO, std::forward<T>(str));
      }

      template<typename... Args>
      inline void warn(fmt::format_string<Args...> fmt_str, Args&&... args) {
        write(SEVERITY_LEVEL::WARN, fmt::format(fmt_str, std::forward<Args>(args)...));
      }

      template<typename T>
      inline void warn(T&& str) 
        requires std::is_convertible_v<std::remove_cvref_t<T>, std::string> {
        write(SEVERITY_LEVEL::WARN, std::forward<T>(str));
      }

      template<typename... Args>
      inline void error(fmt::format_string<Args...> fmt_str, Args&&... args) {
        write(SEVERITY_LEVEL::ERROR, fmt::format(fmt_str, std::forward<Args>(args)...));
      }

      template<typename T>
      inline void error(T&& str) 
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