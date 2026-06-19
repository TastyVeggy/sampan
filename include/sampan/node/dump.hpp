#pragma once

#include <string>

namespace sampan::node {

struct Tree;

[[nodiscard]] std::string dump(const Tree &tree);

} // namespace sampan::node
