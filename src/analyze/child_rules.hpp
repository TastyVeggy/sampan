#pragma once

namespace sampan::node {

enum class NodeKind;

}

namespace sampan::analyze::detail {

[[nodiscard]] bool child_allowed(node::NodeKind parent,
                                 node::NodeKind child) noexcept;

} // namespace sampan::analyze::detail
