#include <gtest/gtest.h>

#include <array>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

#include "sampan/analyze/analyzer.hpp"
#include "sampan/node/dump.hpp"
#include "sampan/parse/parser.hpp"

namespace {

[[nodiscard]] std::string read_file(const std::string &relative_path) {
  std::ifstream input{std::string{SAMPAN_SOURCE_DIR} + relative_path};
  std::stringstream contents;
  contents << input.rdbuf();
  return contents.str();
}

[[nodiscard]] sampan::analyze::AnalyzeResult
analyze_source(std::string source) {
  sampan::parse::ParseResult parsed =
      sampan::parse::parse(std::move(source), 0);
  if (parsed.document == nullptr || !parsed.diagnostics.empty()) {
    return {.tree = nullptr, .diagnostics = std::move(parsed.diagnostics)};
  }
  return sampan::analyze::analyze(*parsed.document);
}

[[nodiscard]] std::string
diagnostics_dump(const std::vector<sampan::core::Diagnostic> &diagnostics) {
  std::ostringstream output;
  for (const sampan::core::Diagnostic &diagnostic : diagnostics) {
    output << diagnostic.span.start_line << ":" << diagnostic.span.start_column
           << ": " << diagnostic.message << "\n";
  }
  return output.str();
}

TEST(Analyzer, ResolvesNodesPropertiesAndValues) {
  const sampan::analyze::AnalyzeResult result = analyze_source(
      "page { stack { gap: 4px text { content: \"Hello\" weight: 600 } } }");

  ASSERT_TRUE(result.diagnostics.empty());
  ASSERT_NE(result.tree, nullptr);
  ASSERT_NE(result.tree->root, nullptr);
  EXPECT_EQ(result.tree->root->kind, sampan::node::NodeKind::Page);

  const auto &stack =
      std::get<sampan::node::ChildNode>(result.tree->root->items.front());
  ASSERT_NE(stack.value, nullptr);
  EXPECT_EQ(stack.value->kind, sampan::node::NodeKind::Stack);
  const auto &gap = std::get<sampan::node::Property>(stack.value->items[0]);
  EXPECT_EQ(gap.id, sampan::node::PropertyId::Gap);
  EXPECT_DOUBLE_EQ(std::get<sampan::node::Length>(gap.value).pixels, 4.0);
}

TEST(Analyzer, SupportsEveryNodeAndProperty) {
  const sampan::analyze::AnalyzeResult result = analyze_source(
      "page {"
      " width: 1px height: 2px"
      " padding: 3px padding-top: 4px padding-right: 5px"
      " padding-bottom: 6px padding-left: 7px"
      " margin: 8px margin-top: 9px margin-right: 10px"
      " margin-bottom: 11px margin-left: 12px"
      " border-width: 13px border-color: #123 background: #abcdef"
      " title: \"All properties\""
      " stack { gap: 1px }"
      " row { gap: 2px align: \"center\" }"
      " box { }"
      " text { content: \"Text\" color: #111 size: 14px weight: 400 }"
      " heading { content: \"Heading\" color: #222 size: 20px level: 2 }"
      " button { }"
      " spacer { size: 5px }"
      " }");

  ASSERT_TRUE(result.diagnostics.empty());
  ASSERT_NE(result.tree, nullptr);
  ASSERT_NE(result.tree->root, nullptr);
  EXPECT_EQ(result.tree->root->items.size(), 23);
}

TEST(Analyzer, RejectsPropertiesOnWrongNodeType) {
  const sampan::analyze::AnalyzeResult result =
      analyze_source("page { box { title: \"No\" } }");

  ASSERT_EQ(result.diagnostics.size(), 1);
  EXPECT_EQ(result.diagnostics.front().message,
            "property 'title' is not valid on node 'box'");
  EXPECT_EQ(result.tree, nullptr);
}

TEST(Analyzer, EnforcesPageRootPlacement) {
  const sampan::analyze::AnalyzeResult wrong_root = analyze_source("stack { }");
  ASSERT_EQ(wrong_root.diagnostics.size(), 1);
  EXPECT_EQ(wrong_root.diagnostics.front().message,
            "document root must be a 'page' node");
  EXPECT_EQ(wrong_root.tree, nullptr);

  const sampan::analyze::AnalyzeResult nested_page =
      analyze_source("page { page { } }");
  ASSERT_EQ(nested_page.diagnostics.size(), 1);
  EXPECT_EQ(nested_page.diagnostics.front().message,
            "node 'page' is only allowed at document root");
  EXPECT_EQ(nested_page.tree, nullptr);
}

TEST(Analyzer, MatchesNodeTreeGoldens) {
  constexpr std::array<std::pair<std::string_view, std::string_view>, 2>
      fixtures{{{"/tests/fixtures/lex/sample.yl",
                 "/tests/golden/node/sample.node.txt"},
                {"/tests/fixtures/parse/nested.yl",
                 "/tests/golden/node/nested.node.txt"}}};

  for (const auto &[fixture_path, golden_path] : fixtures) {
    const sampan::analyze::AnalyzeResult result =
        analyze_source(read_file(std::string{fixture_path}));
    ASSERT_TRUE(result.diagnostics.empty()) << fixture_path;
    ASSERT_NE(result.tree, nullptr) << fixture_path;
    EXPECT_EQ(sampan::node::dump(*result.tree),
              read_file(std::string{golden_path}))
        << fixture_path;
  }
}

TEST(Analyzer, MatchesNegativeFixtureDiagnostics) {
  constexpr std::array<std::string_view, 4> names{
      "unknown_node", "unknown_property", "wrong_property_type",
      "duplicate_property"};

  for (const std::string_view name : names) {
    const std::string fixture_path =
        "/tests/fixtures/errors/" + std::string{name} + ".yl";
    const std::string golden_path =
        "/tests/golden/diagnostics/" + std::string{name} + ".txt";
    const sampan::analyze::AnalyzeResult result =
        analyze_source(read_file(fixture_path));
    EXPECT_EQ(result.tree, nullptr) << name;
    EXPECT_EQ(diagnostics_dump(result.diagnostics), read_file(golden_path))
        << name;
  }
}

} // namespace
