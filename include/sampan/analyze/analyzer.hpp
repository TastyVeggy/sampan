#pragma once

#include <memory>
#include <vector>

#include "sampan/core/diagnostic.hpp"
#include "sampan/node/node.hpp"

namespace sampan::ast {

struct Document;

}

namespace sampan::analyze {

struct AnalyzeResult {
  std::unique_ptr<node::Tree> tree;
  std::vector<core::Diagnostic> diagnostics;
};

[[nodiscard]] AnalyzeResult analyze(const ast::Document &document);

} // namespace sampan::analyze
