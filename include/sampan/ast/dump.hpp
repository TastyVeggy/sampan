#pragma once

#include <string>

#include "sampan/ast/ast.hpp"

namespace sampan::ast {

[[nodiscard]] std::string dump(const Document &document);

} // namespace sampan::ast
