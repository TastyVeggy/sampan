#pragma once

#include <optional>
#include <string_view>

#include "sampan/node/node.hpp"

namespace sampan::analyze::detail {

enum class ValueType { Integer, Float, Length, Color, String, Boolean };

struct PropertyDefinition {
  node::PropertyId id;
  ValueType type;
};

[[nodiscard]] std::optional<node::NodeKind>
resolve_node_kind(std::string_view name) noexcept;
[[nodiscard]] std::optional<PropertyDefinition>
resolve_property(std::string_view name) noexcept;
[[nodiscard]] std::string_view to_string(ValueType type) noexcept;

} // namespace sampan::analyze::detail
