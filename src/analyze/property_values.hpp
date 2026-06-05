#pragma once

#include <expected>
#include <string>

#include "sampan/node/node.hpp"

namespace sampan::analyze::detail {

[[nodiscard]] std::expected<node::Value, std::string>
normalize_alignment(node::Value value);

} // namespace sampan::analyze::detail
