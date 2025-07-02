#include <MR/Logger/WriteRequest.hpp>
#include <MR/Logger/SeverityLevel.hpp>
#include <MR/Interface/ThreadSafeQueue.hpp>
#include <MR/Logger/Logger.hpp>
#include <memory>
#include <stop_token>
#include <string>
#include <thread>

#include <fmt/core.h>
#include <fmt/std.h>
#include <fmt/chrono.h>


namespace MR::Logger {

Logger::Logger(std::shared_ptr<Interface::ThreadSafeQueue<WriteRequest>> q) : 
  file_{"Log.log"},
  ring_{QUEUE_SIZE},
  queue_{q},
  worker_{[this](std::stop_token st){ consume(st); }} {}

void Logger::write(SEVERITY_LEVEL severity, std::string&& str) {

    WriteRequest req{
      .level = severity, 
      .data = std::move(str),
      .threadId = std::this_thread::get_id(),
      .timestamp = std::chrono::system_clock::now()
    };

    queue_->push(std::move(req));
    return;
  }

  void Logger::consume(std::stop_token st) {
    while(!st.stop_requested() || !queue_->empty()) {
      std::cout << "Stop requested = " << st.stop_requested() << std::endl;
      // std::cout << "Queue size = " << queue_->size() << std::endl;

      auto writeRequest = queue_->pop(); 
      if (!writeRequest) continue; // empty optional returned on empty queue so go back to the while check

      std::string msg = fmt::format("[{}] [{}] [Thread: {}]: {}\n",  writeRequest->timestamp, sevLvlToStr(writeRequest->level), writeRequest->threadId, std::move(writeRequest->data));

      ring_.submitWrite(std::make_unique<std::string>(std::move(msg)), file_);
    }
  }

  void Logger::info(std::string&& str) {
    write(SEVERITY_LEVEL::INFO, std::move(str));
  }

  void Logger::warn(std::string&& str) {
    write(SEVERITY_LEVEL::WARN, std::move(str));
  }

  void Logger::error(std::string&& str) {
    write(SEVERITY_LEVEL::ERROR, std::move(str));
  }

  Logger::~Logger() = default;
  
};