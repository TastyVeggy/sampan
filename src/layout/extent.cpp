#include "sampan/layout/layout.hpp"

#include <algorithm>
#include <memory>

namespace sampan::layout {
namespace {

void include_rect(Extent &extent, const Rect &rect) noexcept {
  extent.width = std::max(extent.width, rect.x + rect.width);
  extent.height = std::max(extent.height, rect.y + rect.height);
}

[[nodiscard]] Rect outer_rect(const Dimensions &dimensions) noexcept {
  const Rect &content = dimensions.content;
  const EdgeSizes &padding = dimensions.padding;
  const EdgeSizes &border = dimensions.border;
  const EdgeSizes &margin = dimensions.margin;
  return {
      .x = content.x - padding.left - border.left - margin.left,
      .y = content.y - padding.top - border.top - margin.top,
      .width = margin.left + border.left + padding.left + content.width +
               padding.right + border.right + margin.right,
      .height = margin.top + border.top + padding.top + content.height +
                padding.bottom + border.bottom + margin.bottom,
  };
}

void include_box(Extent &extent, const LayoutBox &box) noexcept {
  include_rect(extent, outer_rect(box.dimensions));
  for (const LayoutItem &item : box.items) {
    if (const auto *text = std::get_if<LayoutText>(&item); text != nullptr) {
      include_rect(extent, text->dimensions);
      for (const LayoutTextLine &line : text->lines) {
        include_rect(extent, line.dimensions);
      }
      continue;
    }
    include_box(extent, *std::get<std::unique_ptr<LayoutBox>>(item));
  }
}

} // namespace

Extent document_extent(const LayoutBox &root) noexcept {
  Extent extent{};
  include_box(extent, root);
  return extent;
}

} // namespace sampan::layout
