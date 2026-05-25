#include "sampan/core/diagnostic.hpp"

namespace sampan::core {

const std::string to_string(const Severity severity) noexcept {
  switch (severity) {
  case Severity::Note:
    return "note";
  case Severity::Warning:
    return "warning";
  case Severity::Error:
    return "error";
  }
  return "unknown";
}

} // namespace sampan::core
