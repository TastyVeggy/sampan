#pragma once
#include <string>

#include "sampan/core/source_span.hpp"

namespace sampan::core {

enum class Severity { Note, Warning, Error };

struct Diagnostic {
  Severity severity{Severity::Error};
  SourceSpan span{};
  std::string message;
};

[[nodiscard]] const std::string to_string(Severity severity) noexcept;

} // namespace sampan::core
