#pragma once

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
      void write(SEVERITY_LEVEL, std::string&&);
      void eventLoop(std::stop_token);
      size_t formatTo(WriteRequest&& msg, char* buffer, size_t capacity);
      Coroutine::WriteTask processRequest(WriteRequest&&);

    public:

      ~Logger();
      
      Logger(const Logger&) = delete;
      Logger(Logger&&) = delete;
      Logger& operator=(Logger&&) = delete;
      Logger& operator=(const Logger&) = delete;

      void info(std::string&& str);
      void info(const std::string& str);
      void warn(std::string&& str);
      void warn(const std::string& str);
      void error(std::string&& str);
      void error(const std::string& str);

      // Factory class for singleton access
      class Factory {
        private:
          friend class Logger;

          static std::shared_ptr<Logger> instance_;
          static std::mutex mutex_;
          static Config stored_config_;
          static bool config_set_;

        public:
          static std::shared_ptr<Logger> _get();
          static void configure(const Config& config);
          static void _reset();
      };

      static std::shared_ptr<Logger> get() { return Factory::_get(); }
  };
}