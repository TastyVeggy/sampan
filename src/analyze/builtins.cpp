#include "builtins.hpp"

#include <array>
#include <ranges>

namespace sampan::analyze::detail {
namespace {

struct NamedNode {
  std::string_view name;
  node::NodeKind kind;
};

struct NamedProperty {
  std::string_view name;
  PropertyDefinition definition;
};

constexpr std::array<NamedNode, 8> kNodes{{
    {"page", node::NodeKind::Page},
    {"stack", node::NodeKind::Stack},
    {"row", node::NodeKind::Row},
    {"box", node::NodeKind::Box},
    {"text", node::NodeKind::Text},
    {"heading", node::NodeKind::Heading},
    {"button", node::NodeKind::Button},
    {"spacer", node::NodeKind::Spacer},
}};

using enum node::PropertyId;
constexpr std::array<NamedProperty, 23> kProperties{{
    {"width", {Width, ValueType::Length}},
    {"height", {Height, ValueType::Length}},
    {"padding", {Padding, ValueType::Length}},
    {"padding-top", {PaddingTop, ValueType::Length}},
    {"padding-right", {PaddingRight, ValueType::Length}},
    {"padding-bottom", {PaddingBottom, ValueType::Length}},
    {"padding-left", {PaddingLeft, ValueType::Length}},
    {"margin", {Margin, ValueType::Length}},
    {"margin-top", {MarginTop, ValueType::Length}},
    {"margin-right", {MarginRight, ValueType::Length}},
    {"margin-bottom", {MarginBottom, ValueType::Length}},
    {"margin-left", {MarginLeft, ValueType::Length}},
    {"border-width", {BorderWidth, ValueType::Length}},
    {"border-color", {BorderColor, ValueType::Color}},
    {"background", {Background, ValueType::Color}},
    {"title", {Title, ValueType::String}},
    {"gap", {Gap, ValueType::Length}},
    {"align", {Align, ValueType::String}},
    {"content", {Content, ValueType::String}},
    {"color", {Color, ValueType::Color}},
    {"size", {Size, ValueType::Length}},
    {"weight", {Weight, ValueType::Integer}},
    {"level", {Level, ValueType::Integer}},
}};

static_assert(kProperties.size() ==
              static_cast<std::size_t>(node::PropertyId::Count));

} // namespace

std::optional<node::NodeKind>
resolve_node_kind(const std::string_view name) noexcept {
  const auto node = std::ranges::find(kNodes, name, &NamedNode::name);
  return node == kNodes.end() ? std::nullopt : std::optional{node->kind};
}

std::optional<PropertyDefinition>
resolve_property(const std::string_view name) noexcept {
  const auto property =
      std::ranges::find(kProperties, name, &NamedProperty::name);
  return property == kProperties.end() ? std::nullopt
                                       : std::optional{property->definition};
}

std::string_view to_string(const ValueType type) noexcept {
  switch (type) {
  case ValueType::Integer:
    return "Integer";
  case ValueType::Float:
    return "Float";
  case ValueType::Length:
    return "Length";
  case ValueType::Color:
    return "Color";
  case ValueType::String:
    return "String";
  case ValueType::Boolean:
    return "Boolean";
  }
  return "Unknown";
}

} // namespace sampan::analyze::detail
