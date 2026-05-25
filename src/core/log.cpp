#include "sampan/core/log.hpp"

#include <ostream>

namespace sampan::core {

Log::Log(std::ostream &output) noexcept : output_(&output) {
}

void Log::set_level(const LogLevel level) noexcept {
  level_ = level;
}

LogLevel Log::level() const noexcept {
  return level_;
}

void Log::write(const LogLevel level, const std::string_view subsystem,
                const std::string_view message) const {
  if (static_cast<int>(level) < static_cast<int>(level_)) {
    return;
  }
  *output_ << to_string(level) << " " << subsystem << " " << message << "\n";
}

const std::string to_string(const LogLevel level) noexcept {
  switch (level) {
  case LogLevel::Trace:
    return "TRACE";
  case LogLevel::Debug:
    return "DEBUG";
  case LogLevel::Info:
    return "INFO";
  case LogLevel::Warn:
    return "WARN";
  case LogLevel::Error:
    return "ERROR";
  }
  return "UNKNOWN";
}

} // namespace sampan::core
