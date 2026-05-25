#pragma once
#include <iosfwd>
#include <string_view>

namespace sampan::core {

enum class LogLevel { Trace, Debug, Info, Warn, Error };

// Synchronous logger
class Log {
public:
  explicit Log(std::ostream &output) noexcept;

  void set_level(LogLevel level) noexcept;

  [[nodiscard]] LogLevel level() const noexcept;

  void write(LogLevel level, std::string_view subsystem,
             std::string_view message) const;

private:
  std::ostream *output_;
  LogLevel level_{LogLevel::Info};
};

[[nodiscard]] const std::string to_string(LogLevel level) noexcept;

} // namespace sampan::core
