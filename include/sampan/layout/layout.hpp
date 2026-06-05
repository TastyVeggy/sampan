#pragma once

#include <memory>
#include <variant>
#include <vector>

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

struct LayoutText {
  const node::TextChild *text;
  const node::Node *owner;
  Rect dimensions;
};

struct LayoutBox;
using LayoutItem = std::variant<LayoutText, std::unique_ptr<LayoutBox>>;

struct LayoutBox {
  const node::Node *node;
  Dimensions dimensions;
  std::vector<LayoutItem> items;
};

[[nodiscard]] std::unique_ptr<LayoutBox> build(const node::Tree &tree,
                                               const Rect &viewport);

} // namespace sampan::layout
