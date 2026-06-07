#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "sampan/layout/text_metrics.hpp"

namespace sampan::layout::detail {

struct WrappedLine {
  std::string text;
  double width;
};

struct WrappedText {
  std::vector<WrappedLine> lines;
  double width;
  double height;
};

[[nodiscard]] double unwrapped_width(std::string_view text,
                                     const TextStyle &style,
                                     const TextMetrics &metrics);
[[nodiscard]] WrappedText wrap_text(std::string_view text,
                                    const TextStyle &style,
                                    double maximum_width,
                                    const TextMetrics &metrics);

} // namespace sampan::layout::detail
