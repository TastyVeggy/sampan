#pragma once

#include <string>

#include "sampan/node/node.hpp"

namespace sampan::node {

[[nodiscard]] std::string dump(const Tree &tree);

} // namespace sampan::node
