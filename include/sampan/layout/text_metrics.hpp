#pragma once

#include <string_view>

namespace sampan::layout {

struct TextStyle {
  double font_size;
  bool bold;
};

class TextMetrics {
public:
  virtual ~TextMetrics() = default;

  [[nodiscard]] virtual double width(std::string_view text,
                                     const TextStyle &style) const = 0;
  [[nodiscard]] virtual double line_height(const TextStyle &style) const = 0;
};

[[nodiscard]] const TextMetrics &fixed_text_metrics() noexcept;

} // namespace sampan::layout
