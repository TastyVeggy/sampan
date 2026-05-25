#include <gtest/gtest.h>

#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "sampan/lex/lexer.hpp"

namespace {

using sampan::lex::Lexer;
using sampan::lex::Token;
using sampan::lex::TokenKind;

std::vector<Token> lex_all(Lexer &lexer) {
  std::vector<Token> tokens;
  do {
    tokens.push_back(lexer.next());
  } while (tokens.back().kind != TokenKind::Eof);
  return tokens;
}

std::vector<TokenKind> kinds(const std::vector<Token> &tokens) {
  std::vector<TokenKind> result;
  for (const Token &token : tokens) {
    result.push_back(token.kind);
  }
  return result;
}

TEST(Lexer, RecognizesStaticDocumentLiteralsAndBooleans) {
  Lexer lexer{
      "-3 3.14 20px #abc #aabbcc \"text\" true false hello-world _name9", 7};
  EXPECT_EQ(kinds(lex_all(lexer)),
            (std::vector<TokenKind>{
                TokenKind::Integer, TokenKind::Float, TokenKind::Length,
                TokenKind::Color, TokenKind::Color, TokenKind::String,
                TokenKind::True, TokenKind::False, TokenKind::Ident,
                TokenKind::Ident, TokenKind::Eof}));
}

TEST(Lexer, RecognizesStaticDocumentPunctuation) {
  Lexer lexer{"{ } :", 0};
  EXPECT_EQ(kinds(lex_all(lexer)),
            (std::vector<TokenKind>{TokenKind::LBrace, TokenKind::RBrace,
                                    TokenKind::Colon, TokenKind::Eof}));
}

TEST(Lexer, SkipsWhitespaceAndComments) {
  Lexer lexer{"// line\n /* block */ \r\nvalue", 3};
  const std::vector<Token> tokens = lex_all(lexer);
  ASSERT_EQ(tokens.size(), 2);
  EXPECT_EQ(tokens.front().kind, TokenKind::Ident);
  EXPECT_EQ(tokens.front().text, "value");
  EXPECT_EQ(tokens.front().span.start_line, 3);
  EXPECT_EQ(tokens.front().span.start_column, 1);
  EXPECT_TRUE(std::move(lexer).diagnostics().empty());
}

TEST(Lexer, RejectsDeferredV2Syntax) {
  Lexer lexer{"20em #abcd \"Hello {name}\" [1] count + 1", 0};
  const std::vector<Token> tokens = lex_all(lexer);
  const std::vector<sampan::core::Diagnostic> diagnostics =
      std::move(lexer).diagnostics();
  EXPECT_EQ(tokens.back().kind, TokenKind::Eof);
  EXPECT_GE(diagnostics.size(), 3);
}

TEST(Lexer, MatchesGoldenTokenDump) {
  std::ifstream fixture{std::string{SAMPAN_SOURCE_DIR} +
                        "/tests/fixtures/lex/sample.yl"};
  std::stringstream source;
  source << fixture.rdbuf();
  Lexer lexer{source.str(), 0};
  std::ostringstream actual;
  for (const Token &token : lex_all(lexer)) {
    actual << sampan::lex::to_string(token.kind) << " " << token.text << "\n";
  }
  std::ifstream expected_file{std::string{SAMPAN_SOURCE_DIR} +
                              "/tests/golden/tokens/sample.tokens.txt"};
  std::stringstream expected;
  expected << expected_file.rdbuf();
  EXPECT_EQ(actual.str(), expected.str());
}

} // namespace
