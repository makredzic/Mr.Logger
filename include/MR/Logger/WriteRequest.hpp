#pragma once
#include <MR/Logger/SeverityLevel.hpp>
#include <string>
#include <thread>
#include <chrono>

namespace MR::Logger {
  struct WriteRequest {
    SEVERITY_LEVEL level;
    std::string data;
    std::thread::id threadId;
    std::chrono::system_clock::time_point timestamp;
  };
}