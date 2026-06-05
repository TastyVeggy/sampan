#pragma once

#include <expected>
#include <optional>
#include <string>
#include <string_view>

#include "sampan/node/node.hpp"

namespace sampan::analyze::detail {

enum class ValueType { Integer, Float, Length, Color, String, Boolean };

using PropertyValueNormalizer =
    std::expected<node::Value, std::string> (*)(node::Value);

struct PropertyDefinition {
  node::PropertyId id;
  ValueType type;
  PropertyValueNormalizer normalize{nullptr};
};

[[nodiscard]] std::optional<node::NodeKind>
resolve_node_kind(std::string_view name) noexcept;
[[nodiscard]] std::optional<PropertyDefinition>
resolve_property(std::string_view name) noexcept;
[[nodiscard]] std::string_view to_string(ValueType type) noexcept;

} // namespace sampan::analyze::detail
