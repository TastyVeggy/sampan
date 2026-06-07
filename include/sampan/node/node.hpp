#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "sampan/core/source_span.hpp"

namespace sampan::node {

enum class NodeKind {
  Page,
  Stack,
  Row,
  Box,
  Text,
  Heading,
  Button,
  Spacer,
};

enum class Alignment { Start, Center, End };

enum class Justification { Start, Center, End };

enum class PropertyId {
  Width,
  Height,
  Padding,
  PaddingTop,
  PaddingRight,
  PaddingBottom,
  PaddingLeft,
  Margin,
  MarginTop,
  MarginRight,
  MarginBottom,
  MarginLeft,
  BorderWidth,
  BorderColor,
  Background,
  Title,
  Gap,
  Align,
  Justify,
  Content,
  Color,
  Size,
  Weight,
  Level,
  Count, // this must always be at the end, used to count the properties when
         // parsing
};

struct Length {
  double pixels;
};

struct Color {
  std::uint8_t red;
  std::uint8_t green;
  std::uint8_t blue;
};

using Value = std::variant<std::int64_t, double, Length, Color, std::string,
                           bool, Alignment, Justification>;

struct Property {
  PropertyId id;
  Value value;
  core::SourceSpan span;
};

struct TextChild {
  std::string value;
  core::SourceSpan span;
};

struct Node;

struct ChildNode {
  std::unique_ptr<Node> value;
};

using Item = std::variant<Property, TextChild, ChildNode>;

struct Node {
  NodeKind kind;
  std::vector<Item> items;
  core::SourceSpan span;
};

struct Tree {
  std::unique_ptr<Node> root;
};

[[nodiscard]] std::string_view to_string(NodeKind kind) noexcept;
[[nodiscard]] std::string_view to_string(PropertyId property) noexcept;
[[nodiscard]] std::string_view to_string(Alignment alignment) noexcept;
[[nodiscard]] std::string_view to_string(Justification justification) noexcept;

} // namespace sampan::node
