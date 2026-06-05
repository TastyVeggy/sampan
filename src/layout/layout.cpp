#include "sampan/layout/layout.hpp"

#include <algorithm>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace sampan::layout {
namespace {

enum class AutoWidth { FillAvailable, FitIntrinsic };

// Layout operates on the typed node tree produced by analysis. These helpers
// read its computed property values without exposing property lookup details to
// the flow algorithms below.

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

[[nodiscard]] const std::string *
string_property(const node::Node &source,
                const node::PropertyId property_id) noexcept {
  const node::Property *property = find_property(source, property_id);
  return property == nullptr ? nullptr
                             : std::get_if<std::string>(&property->value);
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

[[nodiscard]] double font_size(const node::Node &owner) noexcept {
  const double default_size =
      owner.kind == node::NodeKind::Heading ? 28.0 : 16.0;
  return length_property(owner, node::PropertyId::Size).value_or(default_size);
}

[[nodiscard]] double text_height(const node::Node &owner) noexcept {
  return font_size(owner) * 1.2;
}

[[nodiscard]] double text_width(const node::Node &owner,
                                const std::string_view text) noexcept {
  // Tihs is just estimate. The real measurement will come from rendering backend
  return static_cast<double>(text.size()) * font_size(owner) * 0.55;
}

[[nodiscard]] double direct_text_width(const node::Node &source) noexcept {
  double width = 0.0;
  for (const node::Item &item : source.items) {
    if (const auto *text = std::get_if<node::TextChild>(&item);
        text != nullptr) {
      width = std::max(width, text_width(source, text->value));
    }
  }
  return width;
}

[[nodiscard]] std::optional<double>
intrinsic_width(const node::Node &source) noexcept {
  // A missing intrinsic width means that an automatic-width node behaves like
  // a container and consumes the width offered by its parent.
  switch (source.kind) {
  case node::NodeKind::Text:
  case node::NodeKind::Heading:
    if (const std::string *content =
            string_property(source, node::PropertyId::Content);
        content != nullptr) {
      return text_width(source, *content);
    }
    return direct_text_width(source);
  case node::NodeKind::Button:
    return direct_text_width(source);
  case node::NodeKind::Spacer:
    return length_property(source, node::PropertyId::Size).value_or(0.0);
  case node::NodeKind::Page:
  case node::NodeKind::Stack:
  case node::NodeKind::Row:
  case node::NodeKind::Box:
    return std::nullopt;
  }
  return std::nullopt;
}

[[nodiscard]] double intrinsic_height(const node::Node &source,
                                      const bool parent_is_row) noexcept {
  switch (source.kind) {
  case node::NodeKind::Text:
  case node::NodeKind::Heading:
    return text_height(source);
  case node::NodeKind::Button:
    return 36.0;
  case node::NodeKind::Spacer:
    return parent_is_row
               ? 0.0
               : length_property(source, node::PropertyId::Size).value_or(0.0);
  case node::NodeKind::Page:
  case node::NodeKind::Stack:
  case node::NodeKind::Row:
  case node::NodeKind::Box:
    return 0.0;
  }
  return 0.0;
}

[[nodiscard]] node::Alignment alignment(const node::Node &source) noexcept {
  const node::Property *property =
      find_property(source, node::PropertyId::Align);
  if (property == nullptr) {
    return node::Alignment::Start;
  }
  const auto *value = std::get_if<node::Alignment>(&property->value);
  return value == nullptr ? node::Alignment::Start : *value;
}

[[nodiscard]] double outer_width(const LayoutBox &box) noexcept {
  const Dimensions &dimensions = box.dimensions;
  return dimensions.margin.left + dimensions.border.left +
         dimensions.padding.left + dimensions.content.width +
         dimensions.padding.right + dimensions.border.right +
         dimensions.margin.right;
}

[[nodiscard]] double outer_height(const LayoutBox &box) noexcept {
  const Dimensions &dimensions = box.dimensions;
  return dimensions.margin.top + dimensions.border.top +
         dimensions.padding.top + dimensions.content.height +
         dimensions.padding.bottom + dimensions.border.bottom +
         dimensions.margin.bottom;
}

[[nodiscard]] double item_width(const LayoutItem &item) noexcept {
  if (const auto *text = std::get_if<LayoutText>(&item); text != nullptr) {
    return text->dimensions.width;
  }
  return outer_width(*std::get<std::unique_ptr<LayoutBox>>(item));
}

[[nodiscard]] double item_height(const LayoutItem &item) noexcept {
  if (const auto *text = std::get_if<LayoutText>(&item); text != nullptr) {
    return text->dimensions.height;
  }
  return outer_height(*std::get<std::unique_ptr<LayoutBox>>(item));
}

void translate_item(LayoutItem &item, double x, double y);

void translate_box(LayoutBox &box, const double x, const double y) {
  box.dimensions.content.x += x;
  box.dimensions.content.y += y;
  //  must move every descendent as well
  for (LayoutItem &item : box.items) {
    translate_item(item, x, y);
  }
}

void translate_item(LayoutItem &item, const double x, const double y) {
  if (auto *text = std::get_if<LayoutText>(&item); text != nullptr) {
    text->dimensions.x += x;
    text->dimensions.y += y;
    return;
  }
  translate_box(*std::get<std::unique_ptr<LayoutBox>>(item), x, y);
}

[[nodiscard]] std::unique_ptr<LayoutBox>
layout_node(const node::Node &source, const Rect &containing_block,
            bool fills_viewport_height, AutoWidth auto_width,
            bool parent_is_row);

[[nodiscard]] std::optional<LayoutItem>
layout_flow_item(const node::Item &item, const node::Node &owner,
                 const Rect &containing_block, const AutoWidth auto_width,
                 const bool parent_is_row) {
  
  // Properties do not participate in flow. Only direct text and child nodes
  // produce layout items, in the same order in which they appear in the node
  if (const auto *text = std::get_if<node::TextChild>(&item); text != nullptr) {
    const double width =
        auto_width == AutoWidth::FitIntrinsic
            ? std::min(containing_block.width, text_width(owner, text->value))
            : containing_block.width;
    return LayoutText{
        .text = text,
        .owner = &owner,
        .dimensions = {.x = containing_block.x,
                       .y = containing_block.y,
                       .width = width,
                       .height = text_height(owner)},
    };
  }

  const auto *child = std::get_if<node::ChildNode>(&item);
  if (child == nullptr || child->value == nullptr) {
    return std::nullopt;
  }
  return layout_node(*child->value, containing_block, false, auto_width,
                     parent_is_row);
}

[[nodiscard]] double layout_vertical_flow(LayoutBox &result,
                                          const Rect &content,
                                          const double containing_height,
                                          const double gap) {
  // next_item_y is the bottom edge of the last laid-out item. The gap is added
  // before each subsequent flow item but not before the first one.
  double next_item_y = content.y;
  bool has_item = false;
  for (const node::Item &source_item : result.node->items) {
    const bool is_flow_item =
        std::holds_alternative<node::TextChild>(source_item) ||
        std::holds_alternative<node::ChildNode>(source_item);
    if (has_item && is_flow_item) {
      next_item_y += gap;
    }

    std::optional<LayoutItem> item =
        layout_flow_item(source_item, *result.node,
                         {.x = content.x,
                          .y = next_item_y,
                          .width = content.width,
                          .height = containing_height},
                         AutoWidth::FillAvailable, false);
    if (!item.has_value()) {
      continue;
    }

    next_item_y += item_height(*item);
    result.items.push_back(std::move(*item));
    has_item = true;
  }
  return has_item ? next_item_y - content.y : 0.0;
}

void align_line(std::vector<LayoutItem> &items, const std::size_t begin,
                const double line_height, const node::Alignment align) {
  // Row alignment is cross-axis (vertical) alignment. Each item is positioned
  // relative to the tallest item on this particular row line.
  for (std::size_t index = begin; index < items.size(); ++index) {
    const double free_space = line_height - item_height(items[index]);
    double offset = 0.0;
    switch (align) {
    case node::Alignment::Start:
      break;
    case node::Alignment::Center:
      offset = free_space / 2.0;
      break;
    case node::Alignment::End:
      offset = free_space;
      break;
    }
    translate_item(items[index], 0.0, offset);
  }
}

[[nodiscard]] double layout_row_flow(LayoutBox &result, const Rect &content,
                                     const double containing_height,
                                     const double gap) {
  const double content_right = content.x + content.width;

  double line_y = content.y;  // line_y is the top of the current line
  double next_item_x = content.x; // right edge of its most recently placed item
  double line_height = 0.0;
  std::size_t line_begin = 0; // identify the items that must be cross-axis aligned together when line complete
  bool has_line_item = false; 
  bool has_any_item = false;
  const node::Alignment align = alignment(*result.node);

  for (const node::Item &source_item : result.node->items) {
    // first lay out the item at its prosective position so we can get the
    // intrinsic outer width which we then use to see if it fits on this line
    const double candidate_x = next_item_x + (has_line_item ? gap : 0.0);
    const double candidate_y = line_y;
    std::optional<LayoutItem> item =
        layout_flow_item(source_item, *result.node,
                         {.x = candidate_x,
                          .y = line_y,
                          .width = content.width,
                          .height = containing_height},
                         AutoWidth::FitIntrinsic, true);
    if (!item.has_value()) {
      continue;
    }

    const double width = item_width(*item);
    if (has_line_item && candidate_x + width > content_right) {
      // finish and align the current line, the move the newly-built item to
      // the next line
      align_line(result.items, line_begin, line_height, align);
      line_y += line_height + gap;
      next_item_x = content.x;
      line_height = 0.0;
      line_begin = result.items.size();
      has_line_item = false;

      translate_item(
        *item,
        content.x - candidate_x, // move to start of line
        line_y - candidate_y // move to next line
      );
    }

    const double placed_x = has_line_item ? next_item_x + gap : content.x;
    next_item_x = placed_x + width;
    line_height = std::max(line_height, item_height(*item));
    result.items.push_back(std::move(*item));
    has_line_item = true;
    has_any_item = true;
  }

  if (!has_any_item) {
    return 0.0;
  }

  align_line(result.items, line_begin, line_height, align);
  return line_y - content.y + line_height;
}

std::unique_ptr<LayoutBox> layout_node(const node::Node &source,
                                       const Rect &containing_block,
                                       const bool fills_viewport_height,
                                       const AutoWidth auto_width,
                                       const bool parent_is_row) {
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

  // Vertical-flow children normally fill the available width. Row children
  // instead shrink to an intrinsic width so several items can share a line.
  double automatic_width = available_width;
  if (auto_width == AutoWidth::FitIntrinsic) {
    automatic_width = std::min(
        available_width, intrinsic_width(source).value_or(available_width));
  }
  const double content_width = length_property(source, node::PropertyId::Width)
                                   .value_or(automatic_width);
  const double content_x =
      containing_block.x + margin.left + border_size.left + padding.left;
  const double content_y =
      containing_block.y + margin.top + border_size.top + padding.top;

  result->dimensions.content = {
      .x = content_x, .y = content_y, .width = content_width, .height = 0.0};

  const double gap =
      length_property(source, node::PropertyId::Gap).value_or(0.0);
  const double flow_height =
      source.kind == node::NodeKind::Row
          ? layout_row_flow(*result, result->dimensions.content,
                            containing_block.height, gap)
          : layout_vertical_flow(*result, result->dimensions.content,
                                 containing_block.height, gap);

  double automatic_height = intrinsic_height(source, parent_is_row);
  automatic_height = std::max(automatic_height, flow_height);
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
  return layout_node(*tree.root, viewport, true, AutoWidth::FillAvailable,
                     false);
}

} // namespace sampan::layout
