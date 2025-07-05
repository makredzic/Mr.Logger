#pragma once

#include <MR/Coroutine/WriteTask.hpp>
#include <string>

#include <MR/Logger/WriteRequest.hpp>

#include <MR/IO/WriteOnlyFile.hpp>
#include <MR/IO/IOUring.hpp>

#include <MR/Interface/ThreadSafeQueue.hpp>
#include <memory>
#include <thread>

namespace MR::Logger {
  class Logger {
    private:

      // CONFIGURATION
      inline static constexpr uint16_t QUEUE_SIZE{256u};
      inline static constexpr uint16_t BATCH_SIZE{10u};
      inline static constexpr uint16_t MAX_MSGS_PER_ITERATION{10u};

      IO::WriteOnlyFile file_;
      IO::IOUring ring_;
      std::shared_ptr<Interface::ThreadSafeQueue<WriteRequest>> queue_;

      std::jthread worker_;


      void write(SEVERITY_LEVEL, std::string&&);
      void eventLoop(std::stop_token);
      std::string format(WriteRequest&& msg);
      Coroutine::WriteTask processRequest(WriteRequest&&);

    public:

      Logger(std::shared_ptr<Interface::ThreadSafeQueue<WriteRequest>>);
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