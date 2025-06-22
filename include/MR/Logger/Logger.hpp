#pragma once

#include <string>

#include <MR/IO/WriteOnlyFile.hpp>
#include <MR/IO/IOUring.hpp>

#include <MR/Interface/ThreadSafeQueue.hpp>
#include <memory>
#include <thread>

namespace MR::Logger {
  class Logger {
    private:

      inline static uint16_t QUEUE_SIZE = 8u;
      enum class SEVERITY_LEVEL { INFO, WARN, ERROR };

      struct WriteRequest {
        SEVERITY_LEVEL level;
        std::string data;
      };


      IO::IOUring ring_;
      IO::WriteOnlyFile file_;
      std::shared_ptr<Interface::ThreadSafeQueue<WriteRequest>> queue_;

      std::jthread worker_;


      void consume();
      void write(SEVERITY_LEVEL, std::string&&);

    public:

      Logger(std::shared_ptr<Interface::ThreadSafeQueue<WriteRequest>>);
      
      Logger(const Logger&) = delete;
      Logger(Logger&&) = delete;
      Logger& operator=(Logger&&) = delete;
      Logger& operator=(const Logger&) = delete;

      void info(std::string&& str);
      void warn(std::string&& str);
      void error(std::string&& str);
  };
}