#include "sampan/app/document_view.hpp"

#include <QColor>
#include <QFont>
#include <QFontMetricsF>
#include <QPaintEvent>
#include <QPainter>
#include <QRectF>
#include <QResizeEvent>
#include <QScrollBar>
#include <QShowEvent>
#include <QString>
#include <QStyle>

#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "sampan/layout/defaults.hpp"
#include "sampan/layout/layout.hpp"
#include "sampan/node/node.hpp"

namespace sampan::app {
namespace {

constexpr int kScrollSingleStep = 32;

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

[[nodiscard]] std::optional<node::Color>
color_property(const node::Node &source,
               const node::PropertyId property_id) noexcept {
  const node::Property *property = find_property(source, property_id);
  if (property == nullptr) {
    return std::nullopt;
  }
  const auto *color = std::get_if<node::Color>(&property->value);
  return color == nullptr ? std::nullopt : std::optional{*color};
}

[[nodiscard]] std::optional<double>
length_property(const node::Node &source,
                const node::PropertyId property_id) noexcept {
  const node::Property *property = find_property(source, property_id);
  if (property == nullptr) {
    return std::nullopt;
  }
  const auto *length = std::get_if<node::Length>(&property->value);
  return length == nullptr ? std::nullopt : std::optional{length->pixels};
}

[[nodiscard]] QColor to_qcolor(const node::Color color) {
  return {static_cast<int>(color.red), static_cast<int>(color.green),
          static_cast<int>(color.blue)};
}

[[nodiscard]] layout::TextStyle text_style(const node::Node &owner) {
  const layout::TextStyle default_style =
      layout::defaults::text_style(owner.kind);
  return {.font_size = std::max(layout::defaults::kMinimumFontSize,
                                length_property(owner, node::PropertyId::Size)
                                    .value_or(default_style.font_size)),
          .bold = default_style.bold};
}

[[nodiscard]] QFont font_for_style(QFont font, const layout::TextStyle &style) {
  font.setPixelSize(static_cast<int>(style.font_size));
  font.setBold(style.bold);
  return font;
}

class QtTextMetrics final : public layout::TextMetrics {
public:
  QtTextMetrics(QFont base_font, const QPaintDevice *device)
      : base_font_{std::move(base_font)}, device_{device} {
  }

  [[nodiscard]] double width(const std::string_view text,
                             const layout::TextStyle &style) const override {
    const QFontMetricsF metrics{font_for_style(base_font_, style), device_};
    return metrics.horizontalAdvance(
        QString::fromUtf8(text.data(), static_cast<qsizetype>(text.size())));
  }

  [[nodiscard]] double
  line_height(const layout::TextStyle &style) const override {
    const QFontMetricsF metrics{font_for_style(base_font_, style), device_};
    return metrics.height();
  }

private:
  QFont base_font_;
  const QPaintDevice *device_;
};

[[nodiscard]] QRectF padding_rect(const layout::Dimensions &dimensions) {
  const layout::Rect &content = dimensions.content;
  return {content.x - dimensions.padding.left,
          content.y - dimensions.padding.top,
          content.width + dimensions.padding.left + dimensions.padding.right,
          content.height + dimensions.padding.top + dimensions.padding.bottom};
}

[[nodiscard]] QRectF border_rect(const layout::Dimensions &dimensions) {
  const QRectF padding = padding_rect(dimensions);
  return {padding.x() - dimensions.border.left,
          padding.y() - dimensions.border.top,
          padding.width() + dimensions.border.left + dimensions.border.right,
          padding.height() + dimensions.border.top + dimensions.border.bottom};
}

void paint_border(QPainter &painter, const layout::Dimensions &dimensions,
                  const QColor &color) {
  const QRectF outer = border_rect(dimensions);
  const QRectF inner = padding_rect(dimensions);
  painter.fillRect(
      QRectF{outer.left(), outer.top(), outer.width(), dimensions.border.top},
      color);
  painter.fillRect(QRectF{outer.left(), inner.bottom(), outer.width(),
                          dimensions.border.bottom},
                   color);
  painter.fillRect(
      QRectF{outer.left(), inner.top(), dimensions.border.left, inner.height()},
      color);
  painter.fillRect(QRectF{inner.right(), inner.top(), dimensions.border.right,
                          inner.height()},
                   color);
}

void paint_text(QPainter &painter, const node::Node &owner,
                const layout::Rect &dimensions, const std::string &text,
                const QFont &base_font) {
  painter.setFont(font_for_style(base_font, text_style(owner)));
  painter.setPen(
      to_qcolor(color_property(owner, node::PropertyId::Color)
                    .value_or(node::Color{.red = 0, .green = 0, .blue = 0})));
  painter.drawText(
      QRectF{dimensions.x, dimensions.y, dimensions.width, dimensions.height},
      Qt::AlignLeft | Qt::AlignVCenter | Qt::TextSingleLine | Qt::TextDontClip,
      QString::fromStdString(text));
}

void paint_box(QPainter &painter, const layout::LayoutBox &box,
               const QFont &base_font) {
  const node::Node &source = *box.node;
  if (const std::optional<node::Color> background =
          color_property(source, node::PropertyId::Background);
      background.has_value()) {
    painter.fillRect(padding_rect(box.dimensions), to_qcolor(*background));
  }

  if (const std::optional<node::Color> border =
          color_property(source, node::PropertyId::BorderColor);
      border.has_value()) {
    paint_border(painter, box.dimensions, to_qcolor(*border));
  }

  for (const layout::LayoutItem &item : box.items) {
    if (const auto *text = std::get_if<layout::LayoutText>(&item);
        text != nullptr) {
      for (const layout::LayoutTextLine &line : text->lines) {
        paint_text(painter, *text->owner, line.dimensions, line.text,
                   base_font);
      }
      continue;
    }
    paint_box(painter, *std::get<std::unique_ptr<layout::LayoutBox>>(item),
              base_font);
  }
}

[[nodiscard]] int scroll_maximum(const double document_length,
                                 const int viewport_length) noexcept {
  const double overflow =
      std::ceil(document_length) - static_cast<double>(viewport_length);
  if (overflow <= 0.0) {
    return 0;
  }
  if (!std::isfinite(overflow) ||
      overflow >= static_cast<double>(std::numeric_limits<int>::max())) {
    return std::numeric_limits<int>::max();
  }
  return static_cast<int>(overflow);
}

void configure_scroll_bar(QScrollBar &scroll_bar, const double document_length,
                          const int viewport_length) {
  scroll_bar.setPageStep(std::max(0, viewport_length));
  scroll_bar.setSingleStep(kScrollSingleStep);
  scroll_bar.setRange(0, scroll_maximum(document_length, viewport_length));
}

} // namespace

DocumentView::DocumentView(std::unique_ptr<node::Tree> tree, QWidget *parent)
    : QAbstractScrollArea{parent}, tree_{std::move(tree)} {
  setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  resize(960, 640);
}

DocumentView::~DocumentView() = default;

void DocumentView::set_tree(std::unique_ptr<node::Tree> tree) {
  tree_ = std::move(tree);
  horizontalScrollBar()->setValue(0);
  verticalScrollBar()->setValue(0);
  layout_dirty_ = true;
  rebuild_layout();
  viewport()->update();
}

void DocumentView::paintEvent(QPaintEvent *event) {
  if (layout_dirty_) {
    rebuild_layout();
  }

  QPainter painter{viewport()};
  painter.setRenderHint(QPainter::Antialiasing);
  painter.fillRect(event->rect(), Qt::white);
  if (layout_ == nullptr) {
    return;
  }

  painter.translate(-horizontalScrollBar()->value(),
                    -verticalScrollBar()->value());
  paint_box(painter, *layout_, painter.font());
}

void DocumentView::resizeEvent(QResizeEvent *event) {
  QAbstractScrollArea::resizeEvent(event);
  layout_dirty_ = true;
  rebuild_layout();
  viewport()->update();
}

void DocumentView::showEvent(QShowEvent *event) {
  QAbstractScrollArea::showEvent(event);
  rebuild_layout();
}

void DocumentView::scrollContentsBy(const int /* dx */, const int /* dy */) {
  viewport()->update();
}

void DocumentView::rebuild_layout() {
  if (!layout_dirty_ || rebuilding_layout_) {
    return;
  }

  if (tree_ == nullptr) {
    layout_.reset();
    horizontalScrollBar()->setRange(0, 0);
    verticalScrollBar()->setRange(0, 0);
    layout_dirty_ = false;
    return;
  }

  rebuilding_layout_ = true;

  const QSize maximum_viewport = maximumViewportSize();
  const int scroll_bar_extent =
      style()->pixelMetric(QStyle::PM_ScrollBarExtent, nullptr, this);
  const QFont base_font = viewport()->font();
  const QtTextMetrics text_metrics{base_font, viewport()};

  bool needs_horizontal_scroll_bar = false;
  bool needs_vertical_scroll_bar = false;
  int available_width = maximum_viewport.width();
  int available_height = maximum_viewport.height();
  layout::Extent extent;

  // A scrollbar consumes space on the opposite axis. Repeat until adding one
  // scrollbar no longer causes overflow that requires the other scrollbar.
  for (int iteration = 0; iteration < 3; ++iteration) {
    available_width =
        std::max(0, maximum_viewport.width() -
                        (needs_vertical_scroll_bar ? scroll_bar_extent : 0));
    available_height =
        std::max(0, maximum_viewport.height() -
                        (needs_horizontal_scroll_bar ? scroll_bar_extent : 0));

    layout_ = layout::build(*tree_,
                            {.x = 0.0,
                             .y = 0.0,
                             .width = static_cast<double>(available_width),
                             .height = static_cast<double>(available_height)},
                            text_metrics);
    if (layout_ == nullptr) {
      break;
    }

    extent = layout::document_extent(*layout_);
    const bool next_horizontal =
        needs_horizontal_scroll_bar ||
        scroll_maximum(extent.width, available_width) > 0;
    const bool next_vertical =
        needs_vertical_scroll_bar ||
        scroll_maximum(extent.height, available_height) > 0;
    if (next_horizontal == needs_horizontal_scroll_bar &&
        next_vertical == needs_vertical_scroll_bar) {
      break;
    }
    needs_horizontal_scroll_bar = next_horizontal;
    needs_vertical_scroll_bar = next_vertical;
  }

  configure_scroll_bar(*horizontalScrollBar(), extent.width, available_width);
  configure_scroll_bar(*verticalScrollBar(), extent.height, available_height);
  layout_dirty_ = false;
  rebuilding_layout_ = false;
}

} // namespace sampan::app
