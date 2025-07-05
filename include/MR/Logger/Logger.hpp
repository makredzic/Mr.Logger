#pragma once

#include <MR/Logger/Config.hpp>
#include <MR/Coroutine/WriteTask.hpp>
#include <string_view>

#include <string>

#include <MR/Logger/WriteRequest.hpp>
#include <MR/Queue/StdQueue.hpp>

#include <MR/IO/WriteOnlyFile.hpp>
#include <MR/IO/IOUring.hpp>

#include <MR/Interface/ThreadSafeQueue.hpp>
#include <memory>
#include <thread>

namespace MR::Logger {
  using namespace std::literals;

  class Logger {
    private:

      // CONFIGURATION
      inline static Config default_config_{
        .log_file_name = "output.log"sv,
        .info_file_name = ""sv,
        .warn_file_name = ""sv,
        .error_file_name = ""sv,
        .queue_depth = 256u,
        .batch_size = 10u,
        .max_logs_per_iteration = 10u,
        ._queue = std::make_shared<Queue::StdQueue<WriteRequest>>(),
      };

      inline static Config mergeWithDefault(const Config& user_config) {
        return Config{
        .log_file_name = user_config.log_file_name.empty()
            ? default_config_.log_file_name
            : user_config.log_file_name,

        .info_file_name = user_config.info_file_name.empty()
          ? default_config_.info_file_name
          : user_config.info_file_name,

        .warn_file_name = user_config.warn_file_name.empty()
          ? default_config_.warn_file_name
          : user_config.warn_file_name,

        .error_file_name = user_config.error_file_name.empty()
          ? default_config_.error_file_name
          : user_config.error_file_name,

        .queue_depth = user_config.queue_depth == 0
          ? default_config_.queue_depth
          : user_config.queue_depth,

        .batch_size = user_config.batch_size == 0
          ? default_config_.batch_size
          : user_config.batch_size,

        .max_logs_per_iteration = user_config.max_logs_per_iteration == 0
          ? default_config_.max_logs_per_iteration
          : user_config.max_logs_per_iteration,

        ._queue = user_config._queue == nullptr
          ? default_config_._queue
          : user_config._queue
        };
      }
      

      Config config_;
      IO::WriteOnlyFile file_;
      IO::IOUring ring_;
      std::shared_ptr<Interface::ThreadSafeQueue<WriteRequest>> queue_;

      std::jthread worker_;

      void write(SEVERITY_LEVEL, std::string&&);
      void eventLoop(std::stop_token);
      std::string format(WriteRequest&& msg);
      Coroutine::WriteTask processRequest(WriteRequest&&);

    public:

      Logger(const Config& = default_config_);
      ~Logger();
      
      Logger(const Logger&) = delete;
      Logger(Logger&&) = delete;
      Logger& operator=(Logger&&) = delete;
      Logger& operator=(const Logger&) = delete;

      void info(std::string&& str);
      void warn(std::string&& str);
      void error(std::string&& str);
  };
}