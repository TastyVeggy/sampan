#include <gtest/gtest.h>

#include <array>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

#include "sampan/analyze/analyzer.hpp"
#include "sampan/layout/dump.hpp"
#include "sampan/layout/layout.hpp"
#include "sampan/parse/parser.hpp"

namespace {

[[nodiscard]] std::string read_file(const std::string &relative_path) {
  std::ifstream input{std::string{SAMPAN_SOURCE_DIR} + relative_path};
  std::stringstream contents;
  contents << input.rdbuf();
  return contents.str();
}

[[nodiscard]] const sampan::layout::LayoutBox &
box_at(const sampan::layout::LayoutBox &parent, const std::size_t index) {
  return *std::get<std::unique_ptr<sampan::layout::LayoutBox>>(
      parent.items.at(index));
}

[[nodiscard]] const sampan::layout::LayoutText &
text_at(const sampan::layout::LayoutBox &parent, const std::size_t index) {
  return std::get<sampan::layout::LayoutText>(parent.items.at(index));
}

[[nodiscard]] std::unique_ptr<sampan::node::Tree>
analyze_source(std::string source) {
  sampan::parse::ParseResult parsed =
      sampan::parse::parse(std::move(source), 0);
  if (!parsed.diagnostics.empty() || parsed.document == nullptr) {
    return nullptr;
  }
  sampan::analyze::AnalyzeResult analyzed =
      sampan::analyze::analyze(*parsed.document);
  return std::move(analyzed.tree);
}

TEST(Layout, PageFillsViewport) {
  const std::unique_ptr<sampan::node::Tree> tree = analyze_source("page { }");
  ASSERT_NE(tree, nullptr);

  const std::unique_ptr<sampan::layout::LayoutBox> root = sampan::layout::build(
      *tree, {.x = 0.0, .y = 0.0, .width = 800.0, .height = 600.0});

  ASSERT_NE(root, nullptr);
  EXPECT_DOUBLE_EQ(root->dimensions.content.x, 0.0);
  EXPECT_DOUBLE_EQ(root->dimensions.content.y, 0.0);
  EXPECT_DOUBLE_EQ(root->dimensions.content.width, 800.0);
  EXPECT_DOUBLE_EQ(root->dimensions.content.height, 600.0);
}

TEST(Layout, AppliesBoxModelAndExplicitSize) {
  const std::unique_ptr<sampan::node::Tree> tree = analyze_source(
      "page { box { width: 100px height: 50px margin: 5px padding: 10px "
      "border-width: 2px } }");
  ASSERT_NE(tree, nullptr);

  const std::unique_ptr<sampan::layout::LayoutBox> root = sampan::layout::build(
      *tree, {.x = 0.0, .y = 0.0, .width = 800.0, .height = 600.0});
  ASSERT_NE(root, nullptr);
  ASSERT_EQ(root->items.size(), 1);
  const sampan::layout::LayoutBox &box = box_at(*root, 0);

  EXPECT_DOUBLE_EQ(box.dimensions.content.x, 17.0);
  EXPECT_DOUBLE_EQ(box.dimensions.content.y, 17.0);
  EXPECT_DOUBLE_EQ(box.dimensions.content.width, 100.0);
  EXPECT_DOUBLE_EQ(box.dimensions.content.height, 50.0);
}

TEST(Layout, StacksChildrenVerticallyWithGap) {
  const std::unique_ptr<sampan::node::Tree> tree =
      analyze_source("page { stack { gap: 8px box { height: 20px } "
                     "box { height: 30px } } }");
  ASSERT_NE(tree, nullptr);

  const std::unique_ptr<sampan::layout::LayoutBox> root = sampan::layout::build(
      *tree, {.x = 0.0, .y = 0.0, .width = 800.0, .height = 600.0});
  ASSERT_NE(root, nullptr);
  ASSERT_EQ(root->items.size(), 1);
  const sampan::layout::LayoutBox &stack = box_at(*root, 0);
  ASSERT_EQ(stack.items.size(), 2);

  EXPECT_DOUBLE_EQ(box_at(stack, 0).dimensions.content.y, 0.0);
  EXPECT_DOUBLE_EQ(box_at(stack, 1).dimensions.content.y, 28.0);
  EXPECT_DOUBLE_EQ(stack.dimensions.content.height, 58.0);
}

TEST(Layout, UsesIntrinsicTextAndSpacerHeights) {
  const std::unique_ptr<sampan::node::Tree> tree =
      analyze_source("page { stack { text { content: \"Hello\" size: 20px } "
                     "spacer { size: 12px } } }");
  ASSERT_NE(tree, nullptr);

  const std::unique_ptr<sampan::layout::LayoutBox> root = sampan::layout::build(
      *tree, {.x = 0.0, .y = 0.0, .width = 800.0, .height = 600.0});
  ASSERT_NE(root, nullptr);
  const sampan::layout::LayoutBox &stack = box_at(*root, 0);

  EXPECT_DOUBLE_EQ(box_at(stack, 0).dimensions.content.height, 24.0);
  EXPECT_DOUBLE_EQ(box_at(stack, 1).dimensions.content.height, 12.0);
  EXPECT_DOUBLE_EQ(stack.dimensions.content.height, 36.0);
}

TEST(Layout, PlacesTextAndChildNodesInSourceOrder) {
  const std::unique_ptr<sampan::node::Tree> tree = analyze_source(
      "page { box { \"Before\" box { height: 50px } \"After\" } }");
  ASSERT_NE(tree, nullptr);

  const std::unique_ptr<sampan::layout::LayoutBox> root = sampan::layout::build(
      *tree, {.x = 0.0, .y = 0.0, .width = 800.0, .height = 600.0});
  ASSERT_NE(root, nullptr);
  const sampan::layout::LayoutBox &box = box_at(*root, 0);
  ASSERT_EQ(box.items.size(), 3);

  EXPECT_DOUBLE_EQ(text_at(box, 0).dimensions.y, 0.0);
  EXPECT_DOUBLE_EQ(text_at(box, 0).dimensions.height, 19.2);
  EXPECT_DOUBLE_EQ(box_at(box, 1).dimensions.content.y, 19.2);
  EXPECT_DOUBLE_EQ(text_at(box, 2).dimensions.y, 69.2);
  EXPECT_DOUBLE_EQ(box.dimensions.content.height, 88.4);
}

TEST(Layout, MatchesLayoutGoldens) {
  constexpr std::array<std::pair<std::string_view, std::string_view>, 2>
      fixtures{{{"/tests/fixtures/lex/sample.yl",
                 "/tests/golden/layout/sample.layout.txt"},
                {"/tests/fixtures/parse/nested.yl",
                 "/tests/golden/layout/nested.layout.txt"}}};

  for (const auto &[fixture_path, golden_path] : fixtures) {
    const std::unique_ptr<sampan::node::Tree> tree =
        analyze_source(read_file(std::string{fixture_path}));
    ASSERT_NE(tree, nullptr) << fixture_path;

    const std::unique_ptr<sampan::layout::LayoutBox> root =
        sampan::layout::build(
            *tree, {.x = 0.0, .y = 0.0, .width = 1024.0, .height = 768.0});
    ASSERT_NE(root, nullptr) << fixture_path;
    EXPECT_EQ(sampan::layout::dump(*root), read_file(std::string{golden_path}))
        << fixture_path;
  }
}

} // namespace
