#include "sampan/ast/dump.hpp"

#include <iomanip>
#include <sstream>
#include <type_traits>

namespace sampan::ast {
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

void write_literal(std::ostringstream &output, const Literal &literal) {
  std::visit(
      [&output](const auto &value) {
        using T = std::decay_t<decltype(value)>;
        if constexpr (std::is_same_v<T, IntegerLiteral>) {
          output << value.value;
        } else if constexpr (std::is_same_v<T, FloatLiteral>) {
          output << value.value;
        } else if constexpr (std::is_same_v<T, LengthLiteral>) {
          output << value.pixels << "px";
        } else if constexpr (std::is_same_v<T, ColorLiteral>) {
          output << "#" << std::hex << std::setfill('0') << std::setw(2)
                 << static_cast<unsigned int>(value.red) << std::setw(2)
                 << static_cast<unsigned int>(value.green) << std::setw(2)
                 << static_cast<unsigned int>(value.blue) << std::dec;
        } else if constexpr (std::is_same_v<T, StringLiteral>) {
          write_quoted(output, value.value);
        } else {
          output << (value.value ? "true" : "false");
        }
      },
      literal);
}

void write_node(std::ostringstream &output, const Node &node,
                const std::size_t depth) {
  write_indent(output, depth);
  output << "Node " << node.tag << "\n";
  for (const Item &item : node.items) {
    std::visit(
        [&output, depth](const auto &value) {
          using T = std::decay_t<decltype(value)>;
          if constexpr (std::is_same_v<T, Property>) {
            write_indent(output, depth + 1);
            output << "Property " << value.name << " = ";
            write_literal(output, value.value);
            output << "\n";
          } else if constexpr (std::is_same_v<T, TextChild>) {
            write_indent(output, depth + 1);
            output << "Text ";
            write_quoted(output, value.value.value);
            output << "\n";
          } else {
            write_node(output, *value.value, depth + 1);
          }
        },
        item);
  }
}

} // namespace

std::string dump(const Document &document) {
  std::ostringstream output;
  output << "Document\n";
  if (document.root != nullptr) {
    write_node(output, *document.root, 1);
  }
  return output.str();
}

} // namespace sampan::ast
