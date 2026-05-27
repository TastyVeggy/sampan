#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "sampan/ast/ast.hpp"
#include "sampan/core/diagnostic.hpp"

namespace sampan::parse {

struct ParseResult {
  std::unique_ptr<ast::Document> document;
  std::vector<core::Diagnostic> diagnostics;
};

[[nodiscard]] ParseResult parse(std::string &&source, std::uint32_t file_id);

} // namespace sampan::parse
