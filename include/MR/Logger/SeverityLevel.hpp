#pragma once
#include <string>

namespace MR::Logger {
  enum class SEVERITY_LEVEL { INFO, WARN, ERROR };
  inline std::string sevLvlToStr(SEVERITY_LEVEL lvl) {
    switch (lvl) {
      case SEVERITY_LEVEL::INFO: return "INFO";
      case SEVERITY_LEVEL::WARN: return "WARN";
      case SEVERITY_LEVEL::ERROR: return "ERROR";
      default: return "UNKNOWN LEVEL";
    }
  }
}