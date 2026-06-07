#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "sampan/layout/text_metrics.hpp"
#include "sampan/node/node.hpp"

namespace sampan::layout {

struct Rect {
  double x; // left
  double y; // top
  double width;
  double height;
};

struct EdgeSizes {
  double top;
  double right;
  double bottom;
  double left;
};

struct Dimensions {
  Rect content;
  EdgeSizes padding;
  EdgeSizes border;
  EdgeSizes margin;
};

struct LayoutTextLine {
  std::string text;
  Rect dimensions;
};

struct LayoutText {
  std::string_view text;
  const node::Node *owner;
  Rect dimensions;
  std::vector<LayoutTextLine> lines;
};

struct LayoutBox;
using LayoutItem = std::variant<LayoutText, std::unique_ptr<LayoutBox>>;

struct LayoutBox {
  const node::Node *node;
  Dimensions dimensions;
  std::vector<LayoutItem> items;
};

[[nodiscard]] std::unique_ptr<LayoutBox>
build(const node::Tree &tree, const Rect &viewport,
      const TextMetrics &metrics = fixed_text_metrics());

} // namespace sampan::layout
