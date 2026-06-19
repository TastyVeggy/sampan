#pragma once

namespace sampan::node {

enum class NodeKind;
enum class PropertyId;

} // namespace sampan::node

namespace sampan::analyze::detail {

[[nodiscard]] bool property_allowed(node::NodeKind node_kind,
                                    node::PropertyId property) noexcept;

} // namespace sampan::analyze::detail
