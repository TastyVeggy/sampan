#pragma once

#include <string>

namespace sampan::layout {

struct LayoutBox;

[[nodiscard]] std::string dump(const LayoutBox &root);

} // namespace sampan::layout
