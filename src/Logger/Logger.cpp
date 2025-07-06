#include <MR/Logger/Config.hpp>
#include <MR/Logger/WriteRequest.hpp>
#include <MR/Logger/SeverityLevel.hpp>
#include <MR/Interface/ThreadSafeQueue.hpp>
#include <MR/Logger/Logger.hpp>
#include <MR/Coroutine/WriteTask.hpp>


#include <stop_token>
#include <string>
#include <thread>

#include <fmt/core.h>
#include <fmt/std.h>
#include <fmt/chrono.h>


namespace MR::Logger {

Config Logger::mergeWithDefault(const Config& user_config) {
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

Logger::Logger(const Config& config) : 
  config_(mergeWithDefault(config)),
  file_{config_.log_file_name},
  ring_{config_.queue_depth},
  queue_{config_._queue},
  worker_{[this](std::stop_token st){ eventLoop(st); }} {}

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

  void Logger::eventLoop(std::stop_token st) {

    // Required to hold the state of the coroutines while they
    // are suspended and not finished
    std::vector<Coroutine::WriteTask> active_tasks;

    size_t pending_writes = 0;

    while(!st.stop_requested() || !queue_->empty() || !active_tasks.empty()) {
      size_t processed_this_iteration = 0;

      // NON BLOCKING REMOVAL AS LONG AS THERE ARE ELEMENTS
      while (auto request = queue_->tryPop()) { 

        active_tasks.push_back(processRequest(std::move(request.value())));

        pending_writes++;
        processed_this_iteration++;

        if (pending_writes >= config_.batch_size) {
          ring_.submitPendingSQEs();
          pending_writes = 0;
        }

        // Prevents too many requests from causing this loop to stall 
        // and processCompletions not being called below
        if (processed_this_iteration >= config_.max_logs_per_iteration) break;
      }

      // Submit any remaining writes
      if (pending_writes > 0) {
        ring_.submitPendingSQEs();
        pending_writes = 0;
      }
      
      // Process CQEs (may dispatch to thread pool)
      ring_.processCompletions();
      
      // Clean up completed tasks
      active_tasks.erase(
          std::remove_if(active_tasks.begin(), active_tasks.end(),
              [](const Coroutine::WriteTask& task) {
                  return task.done();
              }),
          active_tasks.end()
      );
      
      // Only sleep if we're still running normally
      // During shutdown, process as fast as possible
      if (!st.stop_requested()) {
        std::this_thread::sleep_for(std::chrono::microseconds(100));
      }
    }
  }

  Coroutine::WriteTask Logger::processRequest(WriteRequest&& request) {

    std::string msg = format(std::move(request));

    // Allocate buffer - TODO: Memory buffer pool here (research)
    size_t len = msg.size();
    void* buffer = malloc(len);
    memcpy(buffer, msg.c_str(), len);
    
    // Submit write and wait for completion
    int bytes_written = co_await ring_.write(file_, buffer, len);
    
    // THIS IS RESUMED AFTER THE KERNEL CONSUMES THE CQE
    // Todo: add possible thread pool task
    
   
    free(buffer);
    
    // Handle write result
    if (bytes_written < 0) {
        // Log error or handle failed write
        // Could implement retry logic here
    }
    
    // Could also do other post-write operations:
    // - Update statistics
    // - Notify callbacks
    // - Release semaphores
    // - Update memory pool

  }

  std::string Logger::format(WriteRequest&& writeRequest) {
    return fmt::format(
      "[{}] [{}] [Thread: {}]: {}\n",  
      writeRequest.timestamp, 
      sevLvlToStr(writeRequest.level), 
      writeRequest.threadId, 
      std::move(writeRequest.data)
    );
  }

  void Logger::info(std::string&& str) {
    write(SEVERITY_LEVEL::INFO, std::move(str));
  }

  void Logger::info(const std::string& str) {
    write(SEVERITY_LEVEL::INFO, std::string(str));
  }

  void Logger::warn(std::string&& str) {
    write(SEVERITY_LEVEL::WARN, std::move(str));
  }

  void Logger::warn(const std::string& str) {
    write(SEVERITY_LEVEL::WARN, std::string(str));
  }

  void Logger::error(std::string&& str) {
    write(SEVERITY_LEVEL::ERROR, std::move(str));
  }

  void Logger::error(const std::string& str) {
    write(SEVERITY_LEVEL::ERROR, std::string(str));
  }

  Logger::~Logger() {

    queue_->shutdown();

    if (worker_.joinable()) {
        worker_.request_stop();
        worker_.join(); 
    }
  }

  // Factory static member definitions
  std::shared_ptr<Logger> Logger::Factory::instance_ = nullptr;
  std::mutex Logger::Factory::mutex_;
  Config Logger::Factory::stored_config_ = {};
  bool Logger::Factory::config_set_ = false;

  void Logger::Factory::configure(const Config& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (instance_) {
      // Logger already exists, configuration cannot be changed
      return;
    }
    stored_config_ = config;
    config_set_ = true;
  }

  std::shared_ptr<Logger> Logger::Factory::_get() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!instance_) {
      // Lazy initialization
      if (config_set_) {
        instance_ = std::shared_ptr<Logger>(new Logger(stored_config_));
      } else {
        instance_ = std::shared_ptr<Logger>(new Logger());
      }
    }
    return instance_;
  }

  void Logger::Factory::_reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    instance_.reset();
    config_set_ = false;
    stored_config_ = {};
  }
  
};