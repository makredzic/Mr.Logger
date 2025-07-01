#pragma once

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

      inline static uint16_t QUEUE_SIZE = 8u;

      IO::WriteOnlyFile file_;
      IO::IOUring ring_;
      std::shared_ptr<Interface::ThreadSafeQueue<WriteRequest>> queue_;

      std::jthread worker_;


      void consume(std::stop_token);
      void write(SEVERITY_LEVEL, std::string&&);

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