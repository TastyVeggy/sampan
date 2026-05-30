#pragma once

#include "sampan/node/node.hpp"

namespace sampan::analyze::detail {

[[nodiscard]] bool child_allowed(node::NodeKind parent,
                                 node::NodeKind child) noexcept;

} // namespace sampan::analyze::detail
