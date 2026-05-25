#pragma once
#include <cstdint>

namespace sampan::core {

// Identifies a half-open byte range in a source file
struct SourceSpan {
  std::uint32_t file_id{0};
  std::uint32_t start_offset{0};
  std::uint32_t end_offset{0};

  [[nodiscard]] constexpr bool valid() const noexcept {
    return start_offset <= end_offset;
  }
};

} // namespace sampan::core
