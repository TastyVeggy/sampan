#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "sampan/core/diagnostic.hpp"

namespace sampan::lex {

enum class TokenKind {
  Integer,
  Float,
  Length,
  Color,
  String,
  True,
  False,
  Ident,
  LBrace,
  RBrace,
  Colon,
  Eof,
  Error,
};

struct Token {
  TokenKind kind;
  core::SourceSpan span;
  std::string_view text;
};

[[nodiscard]] std::string_view to_string(TokenKind kind) noexcept;

class Lexer {
public:
  Lexer(std::string &&source, std::uint32_t file_id);

  Lexer(const Lexer &) = delete;
  Lexer &operator=(const Lexer &) = delete;
  Lexer(Lexer &&) = delete;
  Lexer &operator=(Lexer &&) = delete;

  [[nodiscard]] Token next();
  [[nodiscard]] bool at_end() const noexcept;
  [[nodiscard]] std::vector<core::Diagnostic> diagnostics() &&;

private:
  struct Position {
    std::size_t offset;
    std::uint32_t line;
    std::uint32_t column;
  };

  [[nodiscard]] Token scan_number(std::size_t start);
  [[nodiscard]] Token scan_identifier(std::size_t start);
  [[nodiscard]] Token scan_color(std::size_t start);
  [[nodiscard]] Token scan_string(std::size_t start);
  [[nodiscard]] Token make_token(TokenKind kind, std::size_t start) const;
  [[nodiscard]] core::SourceSpan make_span(Position start) const noexcept;
  [[nodiscard]] Token make_error(std::size_t start, std::string message);

  void skip_whitespace_and_comments();
  void advance() noexcept;
  void advance(std::size_t count) noexcept;
  void report(Position start, std::string message);
  [[nodiscard]] bool starts_with(std::string_view text) const noexcept;
  [[nodiscard]] char peek(std::size_t lookahead = 0) const noexcept;
  [[nodiscard]] static bool is_identifier_start(char character) noexcept;
  [[nodiscard]] static bool is_identifier_continue(char character) noexcept;
  [[nodiscard]] static bool is_ascii_digit(char character) noexcept;
  [[nodiscard]] static bool is_hex_digit(char character) noexcept;

  std::string source_;
  std::uint32_t file_id_;
  Position position_{.offset = 0, .line = 1, .column = 1};
  Position token_start_{.offset = 0, .line = 1, .column = 1};
  bool previous_was_carriage_return_{false};
  std::vector<core::Diagnostic> diagnostics_;
};

} // namespace sampan::lex
