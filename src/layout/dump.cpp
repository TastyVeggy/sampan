#include "sampan/layout/dump.hpp"

#include <iomanip>
#include <memory>
#include <sstream>
#include <string_view>

namespace sampan::layout {
namespace {

void write_indent(std::ostringstream &output, const std::size_t depth) {
  output << std::string(depth * 2, ' ');
}

void write_quoted(std::ostringstream &output, const std::string_view value) {
  output << '"';
  for (const char character : value) {
    switch (character) {
    case '\\':
      output << "\\\\";
      break;
    case '"':
      output << "\\\"";
      break;
    case '\n':
      output << "\\n";
      break;
    case '\t':
      output << "\\t";
      break;
    default:
      output << character;
      break;
    }
  }
  output << '"';
}

void write_rect(std::ostringstream &output, const Rect &rect) {
  output << "x=" << rect.x << " y=" << rect.y << " width=" << rect.width
         << " height=" << rect.height;
}

void write_edges(std::ostringstream &output, const EdgeSizes &edges) {
  output << "top=" << edges.top << " right=" << edges.right
         << " bottom=" << edges.bottom << " left=" << edges.left;
}

void write_box(std::ostringstream &output, const LayoutBox &box,
               const std::size_t depth) {
  write_indent(output, depth);
  output << "Box " << node::to_string(box.node->kind) << "\n";

  write_indent(output, depth + 1);
  output << "content ";
  write_rect(output, box.dimensions.content);
  output << "\n";

  write_indent(output, depth + 1);
  output << "padding ";
  write_edges(output, box.dimensions.padding);
  output << "\n";

  write_indent(output, depth + 1);
  output << "border ";
  write_edges(output, box.dimensions.border);
  output << "\n";

  write_indent(output, depth + 1);
  output << "margin ";
  write_edges(output, box.dimensions.margin);
  output << "\n";

  for (const LayoutItem &item : box.items) {
    if (const auto *text = std::get_if<LayoutText>(&item); text != nullptr) {
      write_indent(output, depth + 1);
      output << "Text ";
      write_quoted(output, text->text);
      output << " ";
      write_rect(output, text->dimensions);
      output << "\n";
      for (const LayoutTextLine &line : text->lines) {
        write_indent(output, depth + 2);
        output << "Line ";
        write_quoted(output, line.text);
        output << " ";
        write_rect(output, line.dimensions);
        output << "\n";
      }
      continue;
    }
    write_box(output, *std::get<std::unique_ptr<LayoutBox>>(item), depth + 1);
  }
}

} // namespace

std::string dump(const LayoutBox &root) {
  std::ostringstream output;
  output << std::setprecision(15) << "Layout\n";
  write_box(output, root, 1);
  return output.str();
}

} // namespace sampan::layout
