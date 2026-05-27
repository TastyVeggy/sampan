#include <gtest/gtest.h>

#include <fstream>
#include <sstream>
#include <string>
#include <utility>
#include <variant>

#include "sampan/ast/dump.hpp"
#include "sampan/parse/parser.hpp"

namespace {

using sampan::ast::BooleanLiteral;
using sampan::ast::ChildNode;
using sampan::ast::ColorLiteral;
using sampan::ast::FloatLiteral;
using sampan::ast::IntegerLiteral;
using sampan::ast::LengthLiteral;
using sampan::ast::Property;
using sampan::ast::StringLiteral;
using sampan::ast::TextChild;

TEST(Parser, ParsesPropertiesTextAndNestedNodesInSourceOrder) {
  const sampan::parse::ParseResult result = sampan::parse::parse(
      "page { title: \"Hello\" number: 3 box { width: 10px } \"Welcome\" }", 4);

  ASSERT_TRUE(result.diagnostics.empty());
  ASSERT_NE(result.document, nullptr);
  ASSERT_NE(result.document->root, nullptr);
  const sampan::ast::Node &page = *result.document->root;
  EXPECT_EQ(page.tag, "page");
  ASSERT_EQ(page.items.size(), 4);

  const auto *title = std::get_if<Property>(&page.items[0]);
  ASSERT_NE(title, nullptr);
  EXPECT_EQ(title->name, "title");
  EXPECT_EQ(std::get<StringLiteral>(title->value).value, "Hello");

  const auto *number_property = std::get_if<Property>(&page.items[1]);
  ASSERT_NE(number_property, nullptr);
  EXPECT_EQ(number_property->name, "number");
  EXPECT_EQ(std::get<IntegerLiteral>(number_property->value).value, 3);

  const auto *child = std::get_if<ChildNode>(&page.items[2]);
  ASSERT_NE(child, nullptr);
  ASSERT_NE(child->value, nullptr);
  EXPECT_EQ(child->value->tag, "box");
  ASSERT_EQ(child->value->items.size(), 1);
  const auto *width = std::get_if<Property>(&child->value->items[0]);
  ASSERT_NE(width, nullptr);
  EXPECT_DOUBLE_EQ(std::get<LengthLiteral>(width->value).pixels, 10.0);

  const auto *text = std::get_if<TextChild>(&page.items[3]);
  ASSERT_NE(text, nullptr);
  EXPECT_EQ(text->value.value, "Welcome");
  EXPECT_EQ(text->span.file_id, 4);
}

TEST(Parser, ParsesEveryLiteralKindAndDecodesStrings) {
  const sampan::parse::ParseResult result = sampan::parse::parse(
      "page { integer: -3 float: -0.5 length: 3.25px color: #aBc string: "
      "\"a\\nb\\\"c\" yes: true no: false }",
      0);

  ASSERT_TRUE(result.diagnostics.empty());
  ASSERT_NE(result.document, nullptr);
  const auto &items = result.document->root->items;
  ASSERT_EQ(items.size(), 7);
  EXPECT_EQ(std::get<IntegerLiteral>(std::get<Property>(items[0]).value).value,
            -3);
  EXPECT_DOUBLE_EQ(
      std::get<FloatLiteral>(std::get<Property>(items[1]).value).value, -0.5);
  EXPECT_DOUBLE_EQ(
      std::get<LengthLiteral>(std::get<Property>(items[2]).value).pixels, 3.25);
  const ColorLiteral color =
      std::get<ColorLiteral>(std::get<Property>(items[3]).value);
  EXPECT_EQ(color.red, 0xaa);
  EXPECT_EQ(color.green, 0xbb);
  EXPECT_EQ(color.blue, 0xcc);
  EXPECT_EQ(std::get<StringLiteral>(std::get<Property>(items[4]).value).value,
            "a\nb\"c");
  EXPECT_TRUE(
      std::get<BooleanLiteral>(std::get<Property>(items[5]).value).value);
  EXPECT_FALSE(
      std::get<BooleanLiteral>(std::get<Property>(items[6]).value).value);
}

TEST(Parser, ReportsErrorsAndRecoversAtTheFollowingItem) {
  const sampan::parse::ParseResult result = sampan::parse::parse(
      "page { broken: box { width: 2px } \"still here\" }", 0);

  ASSERT_FALSE(result.diagnostics.empty());
  ASSERT_NE(result.document, nullptr);
  const auto &items = result.document->root->items;
  ASSERT_EQ(items.size(), 2);
  EXPECT_EQ(std::get<ChildNode>(items[0]).value->tag, "box");
  EXPECT_EQ(std::get<TextChild>(items[1]).value.value, "still here");
}

TEST(Parser, ReportsUnterminatedNodesWithoutCrashing) {
  const sampan::parse::ParseResult result =
      sampan::parse::parse("page { box {", 0);

  ASSERT_EQ(result.diagnostics.size(), 2);
  EXPECT_EQ(result.diagnostics[0].message, "expected '}' to close node 'box'");
  EXPECT_EQ(result.diagnostics[1].message, "expected '}' to close node 'page'");
  ASSERT_NE(result.document, nullptr);
  ASSERT_NE(result.document->root, nullptr);
  EXPECT_EQ(result.document->root->tag, "page");
}

TEST(Parser, ReportsMissingOpeningBraceAtTheNodeBoundary) {
  const sampan::parse::ParseResult result =
      sampan::parse::parse("page title: \"Hello\"", 0);

  ASSERT_EQ(result.diagnostics.size(), 1);
  EXPECT_EQ(result.diagnostics.front().message,
            "expected '{' after node name 'page'");
  EXPECT_EQ(result.document, nullptr);
}

TEST(Parser, MatchesGoldenAstDump) {
  std::ifstream fixture{std::string{SAMPAN_SOURCE_DIR} +
                        "/tests/fixtures/lex/sample.yl"};
  std::stringstream source;
  source << fixture.rdbuf();
  const sampan::parse::ParseResult result =
      sampan::parse::parse(source.str(), 0);
  ASSERT_TRUE(result.diagnostics.empty());
  ASSERT_NE(result.document, nullptr);

  std::ifstream expected_file{std::string{SAMPAN_SOURCE_DIR} +
                              "/tests/golden/ast/sample.ast.txt"};
  std::stringstream expected;
  expected << expected_file.rdbuf();
  EXPECT_EQ(sampan::ast::dump(*result.document), expected.str());
}

TEST(Parser, MatchesNestedGoldenAstDump) {
  std::ifstream fixture{std::string{SAMPAN_SOURCE_DIR} +
                        "/tests/fixtures/parse/nested.yl"};
  std::stringstream source;
  source << fixture.rdbuf();
  const sampan::parse::ParseResult result =
      sampan::parse::parse(source.str(), 0);
  ASSERT_TRUE(result.diagnostics.empty());
  ASSERT_NE(result.document, nullptr);

  std::ifstream expected_file{std::string{SAMPAN_SOURCE_DIR} +
                              "/tests/golden/ast/nested.ast.txt"};
  std::stringstream expected;
  expected << expected_file.rdbuf();
  EXPECT_EQ(sampan::ast::dump(*result.document), expected.str());
}

} // namespace
