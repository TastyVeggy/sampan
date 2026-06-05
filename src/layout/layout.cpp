#include "sampan/layout/layout.hpp"

#include <algorithm>
#include <optional>

namespace sampan::layout {
namespace {

[[nodiscard]] const node::Property *
find_property(const node::Node &source,
              const node::PropertyId property_id) noexcept {
  for (const node::Item &item : source.items) {
    const auto *property = std::get_if<node::Property>(&item);
    if (property != nullptr && property->id == property_id) {
      return property;
    }
  }
  return nullptr;
}

[[nodiscard]] std::optional<double>
length_property(const node::Node &source,
                const node::PropertyId property_id) noexcept {
  const node::Property *property = find_property(source, property_id);
  if (property == nullptr) {
    return std::nullopt;
  }

  const auto *length = std::get_if<node::Length>(&property->value);
  if (length == nullptr) {
    return std::nullopt;
  }
  return std::max(0.0, length->pixels);
}

[[nodiscard]] EdgeSizes
edges(const node::Node &source, const node::PropertyId shorthand,
      const node::PropertyId top, const node::PropertyId right,
      const node::PropertyId bottom, const node::PropertyId left) noexcept {
  const double all = length_property(source, shorthand).value_or(0.0);
  return {
      .top = length_property(source, top).value_or(all),
      .right = length_property(source, right).value_or(all),
      .bottom = length_property(source, bottom).value_or(all),
      .left = length_property(source, left).value_or(all),
  };
}

[[nodiscard]] double intrinsic_height(const node::Node &source) noexcept {
  switch (source.kind) {
  case node::NodeKind::Text:
    return length_property(source, node::PropertyId::Size).value_or(16.0) * 1.2;
  case node::NodeKind::Heading:
    return length_property(source, node::PropertyId::Size).value_or(28.0) * 1.2;
  case node::NodeKind::Button:
    return 36.0;
  case node::NodeKind::Spacer:
    return length_property(source, node::PropertyId::Size).value_or(0.0);
  case node::NodeKind::Page:
  case node::NodeKind::Stack:
  case node::NodeKind::Row:
  case node::NodeKind::Box:
    return 0.0;
  }
  return 0.0;
}

[[nodiscard]] double text_height(const node::Node &owner) noexcept {
  const double default_size =
      owner.kind == node::NodeKind::Heading ? 28.0 : 16.0;
  return length_property(owner, node::PropertyId::Size).value_or(default_size) *
         1.2;
}

[[nodiscard]] double outer_height(const LayoutBox &box) noexcept {
  const Dimensions &dimensions = box.dimensions;
  return dimensions.margin.top + dimensions.border.top +
         dimensions.padding.top + dimensions.content.height +
         dimensions.padding.bottom + dimensions.border.bottom +
         dimensions.margin.bottom;
}

[[nodiscard]] std::unique_ptr<LayoutBox>
layout_node(const node::Node &source, const Rect &containing_block,
            const bool fills_viewport_height) {
  auto result = std::make_unique<LayoutBox>();
  result->node = &source;
  result->dimensions.margin =
      edges(source, node::PropertyId::Margin, node::PropertyId::MarginTop,
            node::PropertyId::MarginRight, node::PropertyId::MarginBottom,
            node::PropertyId::MarginLeft);
  result->dimensions.padding =
      edges(source, node::PropertyId::Padding, node::PropertyId::PaddingTop,
            node::PropertyId::PaddingRight, node::PropertyId::PaddingBottom,
            node::PropertyId::PaddingLeft);

  const double border =
      length_property(source, node::PropertyId::BorderWidth).value_or(0.0);
  result->dimensions.border = {
      .top = border, .right = border, .bottom = border, .left = border};

  const EdgeSizes &margin = result->dimensions.margin;
  const EdgeSizes &padding = result->dimensions.padding;
  const EdgeSizes &border_size = result->dimensions.border;
  const double horizontal_edges = margin.left + border_size.left +
                                  padding.left + padding.right +
                                  border_size.right + margin.right;
  const double available_width =
      std::max(0.0, containing_block.width - horizontal_edges);
  const double content_width = length_property(source, node::PropertyId::Width)
                                   .value_or(available_width);
  const double content_x =
      containing_block.x + margin.left + border_size.left + padding.left;
  const double content_y =
      containing_block.y + margin.top + border_size.top + padding.top;

  result->dimensions.content = {
      .x = content_x, .y = content_y, .width = content_width, .height = 0.0};

  double next_item_y = content_y;
  bool has_flow_item = false;
  const double gap =
      length_property(source, node::PropertyId::Gap).value_or(0.0);
  for (const node::Item &item : source.items) {
    const auto *text = std::get_if<node::TextChild>(&item);
    const auto *child = std::get_if<node::ChildNode>(&item);
    if (text == nullptr && (child == nullptr || child->value == nullptr)) {
      continue;
    }

    if (has_flow_item) {
      next_item_y += gap;
    }

    if (text != nullptr) {
      const double height = text_height(source);
      result->items.emplace_back(LayoutText{
          .text = text,
          .owner = &source,
          .dimensions = {.x = content_x,
                         .y = next_item_y,
                         .width = content_width,
                         .height = height},
      });
      next_item_y += height;
    } else {
      std::unique_ptr<LayoutBox> child_box =
          layout_node(*child->value,
                      {.x = content_x,
                       .y = next_item_y,
                       .width = content_width,
                       .height = containing_block.height},
                      false);
      next_item_y += outer_height(*child_box);
      result->items.emplace_back(std::move(child_box));
    }
    has_flow_item = true;
  }

  double automatic_height = intrinsic_height(source);
  if (has_flow_item) {
    automatic_height = std::max(automatic_height, next_item_y - content_y);
  }
  if (fills_viewport_height) {
    const double vertical_edges = margin.top + border_size.top + padding.top +
                                  padding.bottom + border_size.bottom +
                                  margin.bottom;
    automatic_height =
        std::max(automatic_height, containing_block.height - vertical_edges);
  }
  result->dimensions.content.height =
      length_property(source, node::PropertyId::Height)
          .value_or(std::max(0.0, automatic_height));
  return result;
}

} // namespace

std::unique_ptr<LayoutBox> build(const node::Tree &tree, const Rect &viewport) {
  if (tree.root == nullptr) {
    return nullptr;
  }
  return layout_node(*tree.root, viewport, true);
}

} // namespace sampan::layout
