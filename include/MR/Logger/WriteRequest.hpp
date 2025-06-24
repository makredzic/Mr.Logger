#pragma once
#include <MR/Logger/SeverityLevel.hpp>
#include <string>

namespace MR::Logger {
  struct WriteRequest {
    SEVERITY_LEVEL level;
    std::string data;
  };
}