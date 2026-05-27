#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <variant>
#include <vector>

#include "sampan/core/source_span.hpp"

namespace sampan::ast {

struct IntegerLiteral {
  std::int64_t value;
  core::SourceSpan span;
};

struct FloatLiteral {
  double value;
  core::SourceSpan span;
};

struct LengthLiteral {
  double pixels;
  core::SourceSpan span;
};

struct ColorLiteral {
  std::uint8_t red;
  std::uint8_t green;
  std::uint8_t blue;
  core::SourceSpan span;
};

struct StringLiteral {
  std::string value;
  core::SourceSpan span;
};

struct BooleanLiteral {
  bool value;
  core::SourceSpan span;
};

using Literal = std::variant<IntegerLiteral, FloatLiteral, LengthLiteral,
                             ColorLiteral, StringLiteral, BooleanLiteral>;

struct Property {
  std::string name;
  Literal value;
  core::SourceSpan span;
};

struct TextChild {
  StringLiteral value;
  core::SourceSpan span;
};

struct Node;

struct ChildNode {
  std::unique_ptr<Node> value;
};

using Item = std::variant<Property, TextChild, ChildNode>;

struct Node {
  std::string tag;
  std::vector<Item> items;
  core::SourceSpan span;
};

struct Document {
  std::unique_ptr<Node> root;
  core::SourceSpan span;
};

} // namespace sampan::ast
