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
#ifdef LOGGER_TEST_SEQUENCE_TRACKING
    uint64_t sequence_number = 0;
#endif
  };
}