#pragma once

#include <MR/Logger/Config.hpp>
#include <MR/Coroutine/WriteTask.hpp>

#include <string>

#include <MR/Logger/WriteRequest.hpp>
#include <MR/Queue/StdQueue.hpp>

#include <MR/IO/WriteOnlyFile.hpp>
#include <MR/IO/IOUring.hpp>

#include <MR/Interface/ThreadSafeQueue.hpp>
#include <memory>
#include <thread>

namespace MR::Logger {

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
        ._queue = std::make_shared<Queue::StdQueue<WriteRequest>>(),
      };


      Config config_;
      IO::WriteOnlyFile file_;
      IO::IOUring ring_;
      std::shared_ptr<Interface::ThreadSafeQueue<WriteRequest>> queue_;

      std::jthread worker_;

      Config mergeWithDefault(const Config& user_config);
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
      void info(const std::string& str);
      void warn(std::string&& str);
      void warn(const std::string& str);
      void error(std::string&& str);
      void error(const std::string& str);
  };
}