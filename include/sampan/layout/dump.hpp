#pragma once

#include <string>

#include "sampan/layout/layout.hpp"

namespace sampan::layout {

[[nodiscard]] std::string dump(const LayoutBox &root);

} // namespace sampan::layout
