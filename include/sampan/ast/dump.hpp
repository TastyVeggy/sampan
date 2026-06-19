#pragma once

#include <string>

namespace sampan::ast {

struct Document;

[[nodiscard]] std::string dump(const Document &document);

} // namespace sampan::ast
