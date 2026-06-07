#pragma once

#include "sampan/layout/text_metrics.hpp"
#include "sampan/node/node.hpp"

namespace sampan::layout::defaults {

inline constexpr double kMinimumFontSize = 1.0;
inline constexpr TextStyle kBodyTextStyle{
    .font_size = 16.0,
    .bold = false,
};
inline constexpr TextStyle kHeadingTextStyle{
    .font_size = 28.0,
    .bold = true,
};
inline constexpr double kButtonVerticalPadding = 8.0;
inline constexpr double kButtonHorizontalPadding = 12.0;

[[nodiscard]] constexpr TextStyle
text_style(const node::NodeKind kind) noexcept {
  return kind == node::NodeKind::Heading ? kHeadingTextStyle : kBodyTextStyle;
}

} // namespace sampan::layout::defaults
