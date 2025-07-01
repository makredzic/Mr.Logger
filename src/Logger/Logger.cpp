#include <MR/Logger/WriteRequest.hpp>
#include <MR/Logger/SeverityLevel.hpp>
#include <MR/Interface/ThreadSafeQueue.hpp>
#include <MR/Logger/Logger.hpp>
#include <memory>
#include <stop_token>


namespace MR::Logger {

  Logger::Logger(std::shared_ptr<Interface::ThreadSafeQueue<WriteRequest>> q) 
  : file_{"Log.log"}, ring_{QUEUE_SIZE}, queue_{q}, worker_{[this](std::stop_token st){ consume(st); }} {}

  void Logger::write(SEVERITY_LEVEL severity, std::string&& str) {
    queue_->push({severity, std::move(str)});
    return;
  }

  void Logger::consume(std::stop_token st) {
    while(!st.stop_requested()) {

      auto opt = queue_->pop(); 
      if (!opt) return; // empty only if queue shutdown

      // TODO ADD MORE FORMATTING HERE
      std::string msg = std::move(opt->data) + "\n";

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

  Logger::~Logger() {
    queue_->shutdown();

  }
  
};