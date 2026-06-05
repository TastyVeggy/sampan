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

TEST(Layout, PlacesRowItemsHorizontallyWithMarginsAndGap) {
  const std::unique_ptr<sampan::node::Tree> tree = analyze_source(
      "page { row { gap: 10px box { width: 100px height: 20px margin: "
      "5px } box { width: 50px height: 40px } } }");
  ASSERT_NE(tree, nullptr);

  const std::unique_ptr<sampan::layout::LayoutBox> root = sampan::layout::build(
      *tree, {.x = 0.0, .y = 0.0, .width = 500.0, .height = 300.0});
  ASSERT_NE(root, nullptr);
  const sampan::layout::LayoutBox &row = box_at(*root, 0);
  ASSERT_EQ(row.items.size(), 2);

  EXPECT_DOUBLE_EQ(box_at(row, 0).dimensions.content.x, 5.0);
  EXPECT_DOUBLE_EQ(box_at(row, 1).dimensions.content.x, 120.0);
  EXPECT_DOUBLE_EQ(row.dimensions.content.height, 40.0);
}

TEST(Layout, WrapsRowsAndUsesGapBetweenLines) {
  const std::unique_ptr<sampan::node::Tree> tree = analyze_source(
      "page { row { width: 250px gap: 10px box { width: 100px height: "
      "20px } box { width: 100px height: 30px } box { width: 100px "
      "height: 40px } } }");
  ASSERT_NE(tree, nullptr);

  const std::unique_ptr<sampan::layout::LayoutBox> root = sampan::layout::build(
      *tree, {.x = 0.0, .y = 0.0, .width = 500.0, .height = 300.0});
  ASSERT_NE(root, nullptr);
  const sampan::layout::LayoutBox &row = box_at(*root, 0);
  ASSERT_EQ(row.items.size(), 3);

  EXPECT_DOUBLE_EQ(box_at(row, 0).dimensions.content.x, 0.0);
  EXPECT_DOUBLE_EQ(box_at(row, 1).dimensions.content.x, 110.0);
  EXPECT_DOUBLE_EQ(box_at(row, 2).dimensions.content.x, 0.0);
  EXPECT_DOUBLE_EQ(box_at(row, 2).dimensions.content.y, 40.0);
  EXPECT_DOUBLE_EQ(row.dimensions.content.height, 80.0);
}

TEST(Layout, KeepsExactFitsAndTranslatesEveryWrappedLine) {
  const std::unique_ptr<sampan::node::Tree> exact_tree = analyze_source(
      "page { row { width: 210px gap: 10px box { width: 100px height: "
      "10px } box { width: 100px height: 10px } } }");
  ASSERT_NE(exact_tree, nullptr);
  const std::unique_ptr<sampan::layout::LayoutBox> exact_root =
      sampan::layout::build(
          *exact_tree, {.x = 0.0, .y = 0.0, .width = 500.0, .height = 300.0});
  ASSERT_NE(exact_root, nullptr);
  const sampan::layout::LayoutBox &exact_row = box_at(*exact_root, 0);
  EXPECT_DOUBLE_EQ(box_at(exact_row, 1).dimensions.content.x, 110.0);
  EXPECT_DOUBLE_EQ(box_at(exact_row, 1).dimensions.content.y, 0.0);
  EXPECT_DOUBLE_EQ(exact_row.dimensions.content.height, 10.0);

  const std::unique_ptr<sampan::node::Tree> wrapped_tree =
      analyze_source("page { row { width: 130px gap: 10px "
                     "box { width: 60px height: 10px } "
                     "box { width: 60px height: 10px } "
                     "box { width: 60px height: 10px } "
                     "box { width: 60px height: 10px } "
                     "box { width: 60px height: 10px } } }");
  ASSERT_NE(wrapped_tree, nullptr);
  const std::unique_ptr<sampan::layout::LayoutBox> wrapped_root =
      sampan::layout::build(
          *wrapped_tree, {.x = 0.0, .y = 0.0, .width = 500.0, .height = 300.0});
  ASSERT_NE(wrapped_root, nullptr);
  const sampan::layout::LayoutBox &wrapped_row = box_at(*wrapped_root, 0);
  EXPECT_DOUBLE_EQ(box_at(wrapped_row, 2).dimensions.content.y, 20.0);
  EXPECT_DOUBLE_EQ(box_at(wrapped_row, 3).dimensions.content.x, 70.0);
  EXPECT_DOUBLE_EQ(box_at(wrapped_row, 3).dimensions.content.y, 20.0);
  EXPECT_DOUBLE_EQ(box_at(wrapped_row, 4).dimensions.content.x, 0.0);
  EXPECT_DOUBLE_EQ(box_at(wrapped_row, 4).dimensions.content.y, 40.0);
  EXPECT_DOUBLE_EQ(wrapped_row.dimensions.content.height, 50.0);
}

TEST(Layout, AlignsRowItemsWithinEachLine) {
  const std::unique_ptr<sampan::node::Tree> centered_tree = analyze_source(
      "page { row { align: \"center\" box { width: 50px height: 20px } "
      "box { width: 50px height: 40px } } }");
  ASSERT_NE(centered_tree, nullptr);
  const std::unique_ptr<sampan::layout::LayoutBox> centered_root =
      sampan::layout::build(
          *centered_tree,
          {.x = 0.0, .y = 0.0, .width = 500.0, .height = 300.0});
  ASSERT_NE(centered_root, nullptr);
  const sampan::layout::LayoutBox &centered_row = box_at(*centered_root, 0);
  EXPECT_DOUBLE_EQ(box_at(centered_row, 0).dimensions.content.y, 10.0);
  EXPECT_DOUBLE_EQ(box_at(centered_row, 1).dimensions.content.y, 0.0);

  const std::unique_ptr<sampan::node::Tree> ended_tree = analyze_source(
      "page { row { align: \"end\" box { width: 50px height: 20px } "
      "box { width: 50px height: 40px } } }");
  ASSERT_NE(ended_tree, nullptr);
  const std::unique_ptr<sampan::layout::LayoutBox> ended_root =
      sampan::layout::build(
          *ended_tree, {.x = 0.0, .y = 0.0, .width = 500.0, .height = 300.0});
  ASSERT_NE(ended_root, nullptr);
  const sampan::layout::LayoutBox &ended_row = box_at(*ended_root, 0);
  EXPECT_DOUBLE_EQ(box_at(ended_row, 0).dimensions.content.y, 20.0);
  EXPECT_DOUBLE_EQ(box_at(ended_row, 1).dimensions.content.y, 0.0);
}

TEST(Layout, AlignsWrappedLinesAndKeepsOversizedItems) {
  const std::unique_ptr<sampan::node::Tree> aligned_tree =
      analyze_source("page { row { width: 130px gap: 10px align: \"end\" "
                     "box { width: 60px height: 10px } "
                     "box { width: 60px height: 20px } "
                     "box { width: 60px height: 30px } "
                     "box { width: 60px height: 10px } } }");
  ASSERT_NE(aligned_tree, nullptr);
  const std::unique_ptr<sampan::layout::LayoutBox> aligned_root =
      sampan::layout::build(
          *aligned_tree, {.x = 0.0, .y = 0.0, .width = 500.0, .height = 300.0});
  ASSERT_NE(aligned_root, nullptr);
  const sampan::layout::LayoutBox &aligned_row = box_at(*aligned_root, 0);
  EXPECT_DOUBLE_EQ(box_at(aligned_row, 0).dimensions.content.y, 10.0);
  EXPECT_DOUBLE_EQ(box_at(aligned_row, 1).dimensions.content.y, 0.0);
  EXPECT_DOUBLE_EQ(box_at(aligned_row, 2).dimensions.content.y, 30.0);
  EXPECT_DOUBLE_EQ(box_at(aligned_row, 3).dimensions.content.y, 50.0);
  EXPECT_DOUBLE_EQ(aligned_row.dimensions.content.height, 60.0);

  const std::unique_ptr<sampan::node::Tree> oversized_tree =
      analyze_source("page { row { width: 100px gap: 10px "
                     "box { width: 150px height: 10px } "
                     "box { width: 20px height: 20px } } }");
  ASSERT_NE(oversized_tree, nullptr);
  const std::unique_ptr<sampan::layout::LayoutBox> oversized_root =
      sampan::layout::build(
          *oversized_tree,
          {.x = 0.0, .y = 0.0, .width = 500.0, .height = 300.0});
  ASSERT_NE(oversized_root, nullptr);
  const sampan::layout::LayoutBox &oversized_row = box_at(*oversized_root, 0);
  EXPECT_DOUBLE_EQ(box_at(oversized_row, 0).dimensions.content.x, 0.0);
  EXPECT_DOUBLE_EQ(box_at(oversized_row, 0).dimensions.content.y, 0.0);
  EXPECT_DOUBLE_EQ(box_at(oversized_row, 1).dimensions.content.x, 0.0);
  EXPECT_DOUBLE_EQ(box_at(oversized_row, 1).dimensions.content.y, 20.0);
  EXPECT_DOUBLE_EQ(oversized_row.dimensions.content.height, 40.0);
}

TEST(Layout, UsesIntrinsicWidthsForRowLeavesAndText) {
  const std::unique_ptr<sampan::node::Tree> tree = analyze_source(
      "page { row { \"Hi\" spacer { size: 10px } text { content: \"AB\" "
      "} } }");
  ASSERT_NE(tree, nullptr);

  const std::unique_ptr<sampan::layout::LayoutBox> root = sampan::layout::build(
      *tree, {.x = 0.0, .y = 0.0, .width = 500.0, .height = 300.0});
  ASSERT_NE(root, nullptr);
  const sampan::layout::LayoutBox &row = box_at(*root, 0);
  ASSERT_EQ(row.items.size(), 3);

  EXPECT_DOUBLE_EQ(text_at(row, 0).dimensions.width, 17.6);
  EXPECT_DOUBLE_EQ(box_at(row, 1).dimensions.content.x, 17.6);
  EXPECT_DOUBLE_EQ(box_at(row, 1).dimensions.content.width, 10.0);
  EXPECT_DOUBLE_EQ(box_at(row, 1).dimensions.content.height, 0.0);
  EXPECT_DOUBLE_EQ(box_at(row, 2).dimensions.content.x, 27.6);
  EXPECT_DOUBLE_EQ(box_at(row, 2).dimensions.content.width, 17.6);
}

TEST(Layout, MatchesLayoutGoldens) {
  constexpr std::array<std::string_view, 8> fixtures{
      "sample",
      "nested",
      "row_wrapping",
      "row_exact_fit",
      "row_oversized_item",
      "row_wrapped_alignment",
      "asymmetric_edges",
      "empty_and_spacers",
  };

  for (const std::string_view fixture : fixtures) {
    const std::string fixture_path =
        "/tests/fixtures/layout/" + std::string{fixture} + ".yl";
    const std::string golden_path =
        "/tests/golden/layout/" + std::string{fixture} + ".layout.txt";
    const std::unique_ptr<sampan::node::Tree> tree =
        analyze_source(read_file(fixture_path));
    ASSERT_NE(tree, nullptr) << fixture_path;

    const std::unique_ptr<sampan::layout::LayoutBox> root =
        sampan::layout::build(
            *tree, {.x = 0.0, .y = 0.0, .width = 1024.0, .height = 768.0});
    ASSERT_NE(root, nullptr) << fixture_path;
    EXPECT_EQ(sampan::layout::dump(*root), read_file(golden_path))
        << fixture_path;
  }
}

} // namespace
