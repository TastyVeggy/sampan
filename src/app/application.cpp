#include "sampan/app/application.hpp"

#include <QApplication>
#include <QColor>
#include <QFileInfo>
#include <QFont>
#include <QPainter>
#include <QRectF>
#include <QString>
#include <QWidget>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "sampan/analyze/analyzer.hpp"
#include "sampan/layout/layout.hpp"
#include "sampan/parse/parser.hpp"

namespace sampan::app {
namespace {

constexpr int kUserProgramError = 3;
constexpr int kIoError = 5;

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

[[nodiscard]] std::optional<std::string>
string_property(const node::Node &source, const node::PropertyId property_id) {
  const node::Property *property = find_property(source, property_id);
  if (property == nullptr) {
    return std::nullopt;
  }
  const auto *text = std::get_if<std::string>(&property->value);
  return text == nullptr ? std::nullopt : std::optional{*text};
}

[[nodiscard]] QColor to_qcolor(const node::Color color) {
  return {static_cast<int>(color.red), static_cast<int>(color.green),
          static_cast<int>(color.blue)};
}

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
                const layout::Rect &dimensions, const std::string &text) {
  QFont font = painter.font();
  const double default_size =
      owner.kind == node::NodeKind::Heading ? 28.0 : 16.0;
  font.setPixelSize(static_cast<int>(std::max(
      1.0,
      length_property(owner, node::PropertyId::Size).value_or(default_size))));
  font.setBold(owner.kind == node::NodeKind::Heading);
  painter.setFont(font);
  painter.setPen(
      to_qcolor(color_property(owner, node::PropertyId::Color)
                    .value_or(node::Color{.red = 0, .green = 0, .blue = 0})));
  painter.drawText(
      QRectF{dimensions.x, dimensions.y, dimensions.width, dimensions.height},
      Qt::AlignLeft | Qt::AlignVCenter | Qt::TextWordWrap,
      QString::fromStdString(text));
}

void paint_box(QPainter &painter, const layout::LayoutBox &box) {
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

  if (const std::optional<std::string> text =
          string_property(source, node::PropertyId::Content);
      text.has_value()) {
    paint_text(painter, source, box.dimensions.content, *text);
  }

  for (const layout::LayoutItem &item : box.items) {
    if (const auto *text = std::get_if<layout::LayoutText>(&item);
        text != nullptr) {
      paint_text(painter, *text->owner, text->dimensions, text->text->value);
      continue;
    }
    paint_box(painter, *std::get<std::unique_ptr<layout::LayoutBox>>(item));
  }
}

void print_diagnostics(const char *source_path,
                       const std::vector<core::Diagnostic> &diagnostics) {
  for (const core::Diagnostic &diagnostic : diagnostics) {
    std::cerr << source_path << ":" << diagnostic.span.start_line << ":"
              << diagnostic.span.start_column << ": "
              << core::to_string(diagnostic.severity) << ": "
              << diagnostic.message << "\n";
  }
}

class DocumentView final : public QWidget {
public:
  explicit DocumentView(std::unique_ptr<node::Tree> tree)
      : tree_{std::move(tree)} {
    setMinimumSize(320, 240);
    resize(960, 640);
  }

protected:
  void paintEvent(QPaintEvent *event) override {
    QWidget::paintEvent(event);
    QPainter painter{this};
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(), Qt::white);
    const std::unique_ptr<layout::LayoutBox> root =
        layout::build(*tree_, {.x = 0.0,
                               .y = 0.0,
                               .width = static_cast<double>(width()),
                               .height = static_cast<double>(height())});
    if (root != nullptr) {
      paint_box(painter, *root);
    }
  }

private:
  std::unique_ptr<node::Tree> tree_;
};

} // namespace

int run(int argc, char *argv[], const char *source_path) {
  std::ifstream input{source_path, std::ios::binary};
  if (!input) {
    std::cerr << "error: cannot open " << source_path << "\n";
    return kIoError;
  }

  std::string source{std::istreambuf_iterator<char>{input},
                     std::istreambuf_iterator<char>{}};
  parse::ParseResult parsed = parse::parse(std::move(source), 0);
  if (!parsed.diagnostics.empty() || parsed.document == nullptr) {
    print_diagnostics(source_path, parsed.diagnostics);
    return kUserProgramError;
  }

  analyze::AnalyzeResult analyzed = analyze::analyze(*parsed.document);
  if (!analyzed.diagnostics.empty() || analyzed.tree == nullptr) {
    print_diagnostics(source_path, analyzed.diagnostics);
    return kUserProgramError;
  }

  QApplication application{argc, argv};
  DocumentView view{std::move(analyzed.tree)};
  view.setWindowTitle(QFileInfo{QString::fromUtf8(source_path)}.fileName() +
                      " — Sampan");
  view.show();
  return application.exec();
}

} // namespace sampan::app
