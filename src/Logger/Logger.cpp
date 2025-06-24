#include <MR/Logger/WriteRequest.hpp>
#include <MR/Logger/SeverityLevel.hpp>
#include <MR/Interface/ThreadSafeQueue.hpp>
#include <MR/Logger/Logger.hpp>
#include <stop_token>


namespace MR::Logger {

  Logger::Logger(std::shared_ptr<Interface::ThreadSafeQueue<WriteRequest>> q) 
  : ring_{QUEUE_SIZE}, file_{"Log.log"}, queue_{q}, worker_{[this](std::stop_token st){ consume(st); }} {}

  void Logger::write(SEVERITY_LEVEL severity, std::string&& str) {
    queue_->push({severity, std::move(str)});
    return;
  }

  void Logger::consume(std::stop_token st) {
    while(!st.stop_requested()) {

      auto opt = queue_->pop(); // nullopt only if queue shutdown
      if (!opt) return;

      std::string msg = std::move(opt->data) + "\n";
      ring_.submitWrite(msg, file_);

      // TODO DELETE - used only for testing to preserve messages since thats not implemented yet
      DELETE_ME_Q.push(std::move(msg));

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