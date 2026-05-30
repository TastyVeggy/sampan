#pragma once

#include "sampan/node/node.hpp"

namespace sampan::analyze::detail {

[[nodiscard]] bool property_allowed(node::NodeKind node_kind,
                                    node::PropertyId property) noexcept;

} // namespace sampan::analyze::detail
