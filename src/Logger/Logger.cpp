#include <MR/Interface/ThreadSafeQueue.hpp>
#include <MR/Logger/Logger.hpp>


namespace MR::Logger {

  Logger::Logger(std::shared_ptr<Interface::ThreadSafeQueue<WriteRequest>> q) 
  : ring_{QUEUE_SIZE}, file_{"Log.log"}, queue_{q}, worker_{&Logger::consume, this} {}

  void Logger::write(SEVERITY_LEVEL severity, std::string&& str) {
    queue_->push({severity, std::move(str)});
    return;
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
  
};