#include "sampan/node/dump.hpp"

#include <iomanip>
#include <sstream>

#include "sampan/core/overloaded.hpp"

namespace sampan::node {
namespace {

void write_indent(std::ostringstream &output, const std::size_t depth) {
  output << std::string(depth * 2, ' ');
}

void write_quoted(std::ostringstream &output, const std::string &value) {
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

void write_value(std::ostringstream &output, const Value &value) {
  std::visit(
      core::Overloaded{
          [&output](const std::int64_t resolved) { output << resolved; },
          [&output](const double resolved) { output << resolved; },
          [&output](const Length resolved) {
            output << resolved.pixels << "px";
          },
          [&output](const Color resolved) {
            output << "#" << std::hex << std::setfill('0') << std::setw(2)
                   << static_cast<unsigned int>(resolved.red) << std::setw(2)
                   << static_cast<unsigned int>(resolved.green) << std::setw(2)
                   << static_cast<unsigned int>(resolved.blue) << std::dec;
          },
          [&output](const std::string &resolved) {
            write_quoted(output, resolved);
          },
          [&output](const bool resolved) {
            output << (resolved ? "true" : "false");
          }},
      value);
}

void write_node(std::ostringstream &output, const Node &node,
                const std::size_t depth) {
  write_indent(output, depth);
  output << "Node " << to_string(node.kind) << "\n";
  for (const Item &item : node.items) {
    std::visit(core::Overloaded{[&output, depth](const Property &resolved) {
                                  write_indent(output, depth + 1);
                                  output << "Property "
                                         << to_string(resolved.id) << " = ";
                                  write_value(output, resolved.value);
                                  output << "\n";
                                },
                                [&output, depth](const TextChild &resolved) {
                                  write_indent(output, depth + 1);
                                  output << "Text ";
                                  write_quoted(output, resolved.value);
                                  output << "\n";
                                },
                                [&output, depth](const ChildNode &resolved) {
                                  write_node(output, *resolved.value,
                                             depth + 1);
                                }},
               item);
  }
}

} // namespace

std::string_view to_string(const NodeKind kind) noexcept {
  switch (kind) {
  case NodeKind::Page:
    return "page";
  case NodeKind::Stack:
    return "stack";
  case NodeKind::Row:
    return "row";
  case NodeKind::Box:
    return "box";
  case NodeKind::Text:
    return "text";
  case NodeKind::Heading:
    return "heading";
  case NodeKind::Button:
    return "button";
  case NodeKind::Spacer:
    return "spacer";
  }
  return "unknown";
}

std::string_view to_string(const PropertyId property) noexcept {
  switch (property) {
  case PropertyId::Width:
    return "width";
  case PropertyId::Height:
    return "height";
  case PropertyId::Padding:
    return "padding";
  case PropertyId::PaddingTop:
    return "padding-top";
  case PropertyId::PaddingRight:
    return "padding-right";
  case PropertyId::PaddingBottom:
    return "padding-bottom";
  case PropertyId::PaddingLeft:
    return "padding-left";
  case PropertyId::Margin:
    return "margin";
  case PropertyId::MarginTop:
    return "margin-top";
  case PropertyId::MarginRight:
    return "margin-right";
  case PropertyId::MarginBottom:
    return "margin-bottom";
  case PropertyId::MarginLeft:
    return "margin-left";
  case PropertyId::BorderWidth:
    return "border-width";
  case PropertyId::BorderColor:
    return "border-color";
  case PropertyId::Background:
    return "background";
  case PropertyId::Title:
    return "title";
  case PropertyId::Gap:
    return "gap";
  case PropertyId::Align:
    return "align";
  case PropertyId::Content:
    return "content";
  case PropertyId::Color:
    return "color";
  case PropertyId::Size:
    return "size";
  case PropertyId::Weight:
    return "weight";
  case PropertyId::Level:
    return "level";
  case PropertyId::Count:
    break;
  }
  return "unknown";
}

std::string dump(const Tree &tree) {
  std::ostringstream output;
  output << "Tree\n";
  if (tree.root != nullptr) {
    write_node(output, *tree.root, 1);
  }
  return output.str();
}

} // namespace sampan::node
