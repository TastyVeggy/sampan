#include "sampan/lex/lexer.hpp"

#include <utility>

namespace sampan::lex {

Lexer::Lexer(std::string &&source, const std::uint32_t file_id)
    : source_(std::move(source)), file_id_(file_id) {
}

Token Lexer::next() {
  skip_whitespace_and_comments();
  token_start_ = position_;
  const std::size_t start = position_.offset;
  if (at_end())
    return make_token(TokenKind::Eof, start);
  const char character = peek();
  if (is_ascii_digit(character) ||
      (character == '-' && is_ascii_digit(peek(1))))
    return scan_number(start);
  if (is_identifier_start(character))
    return scan_identifier(start);
  advance();
  switch (character) {
  case '#':
    return scan_color(start);
  case '"':
    return scan_string(start);
  case '{':
    return make_token(TokenKind::LBrace, start);
  case '}':
    return make_token(TokenKind::RBrace, start);
  case ':':
    return make_token(TokenKind::Colon, start);
  default:
    return make_error(start, "invalid character in Yuloh document");
  }
}

bool Lexer::at_end() const noexcept {
  return position_.offset >= source_.size();
}

std::vector<core::Diagnostic> Lexer::diagnostics() && {
  return std::move(diagnostics_);
}

Token Lexer::scan_number(const std::size_t start) {
  if (peek() == '-')
    advance();
  while (is_ascii_digit(peek()))
    advance();
  bool is_float = false;
  if (peek() == '.' && is_ascii_digit(peek(1))) {
    is_float = true;
    advance();
    while (is_ascii_digit(peek()))
      advance();
  }
  if (starts_with("px")) {
    advance(2);
    return make_token(TokenKind::Length, start);
  }
  return make_token(is_float ? TokenKind::Float : TokenKind::Integer, start);
}

Token Lexer::scan_identifier(const std::size_t start) {
  while (is_identifier_continue(peek()))
    advance();
  const std::string_view text =
      std::string_view{source_}.substr(start, position_.offset - start);
  if (text == "true")
    return make_token(TokenKind::True, start);
  if (text == "false")
    return make_token(TokenKind::False, start);
  return make_token(TokenKind::Ident, start);
}

Token Lexer::scan_color(const std::size_t start) {
  const std::size_t digits_start = position_.offset;
  while (is_hex_digit(peek()))
    advance();
  const std::size_t count = position_.offset - digits_start;
  if (count == 3 || count == 6)
    return make_token(TokenKind::Color, start);
  return make_error(start,
                    "color literal must contain 3 or 6 hexadecimal digits");
}

Token Lexer::scan_string(const std::size_t start) {
  while (!at_end()) {
    const char character = peek();
    if (character == '"') {
      advance();
      return make_token(TokenKind::String, start);
    }
    if (character == '\n' || character == '\r')
      return make_error(start, "unterminated string literal");
    if (character == '\\') {
      advance();
      const char escaped = peek();
      if (escaped != 'n' && escaped != 't' && escaped != '\\' &&
          escaped != '"' && escaped != '{' && escaped != '}') {
        while (!at_end() && peek() != '"' && peek() != '\n' && peek() != '\r')
          advance();
        if (peek() == '"')
          advance();
        return make_error(start, "invalid string escape");
      }
      advance();
      continue;
    }
    if (character == '{' || character == '}')
      return make_error(start,
                        "string interpolation is not available in Yuloh v1");
    advance();
  }
  return make_error(start, "unterminated string literal");
}

Token Lexer::make_token(const TokenKind kind, const std::size_t start) const {
  return {.kind = kind,
          .span = make_span(token_start_),
          .text = std::string_view{source_}.substr(start,
                                                   position_.offset - start)};
}

core::SourceSpan Lexer::make_span(const Position start) const noexcept {
  return {.file_id = file_id_,
          .start_offset = static_cast<std::uint32_t>(start.offset),
          .end_offset = static_cast<std::uint32_t>(position_.offset),
          .start_line = start.line,
          .start_column = start.column,
          .end_line = position_.line,
          .end_column = position_.column};
}

Token Lexer::make_error(const std::size_t start, std::string message) {
  report(token_start_, std::move(message));
  return make_token(TokenKind::Error, start);
}

void Lexer::skip_whitespace_and_comments() {
  while (!at_end()) {
    const char character = peek();
    if (character == ' ' || character == '\t' || character == '\n' ||
        character == '\r') {
      advance();
      continue;
    }
    if (starts_with("//")) {
      advance(2);
      while (!at_end() && peek() != '\n' && peek() != '\r')
        advance();
      continue;
    }
    if (starts_with("/*")) {
      const Position start = position_;
      advance(2);
      while (!at_end() && !starts_with("*/"))
        advance();
      if (at_end()) {
        report(start, "unterminated block comment");
        return;
      }
      advance(2);
      continue;
    }
    break;
  }
}

void Lexer::advance() noexcept {
  if (at_end())
    return;
  const char character = source_[position_.offset++];
  if (character == '\r') {
    ++position_.line;
    position_.column = 1;
    previous_was_carriage_return_ = true;
  } else if (character == '\n') {
    if (!previous_was_carriage_return_)
      ++position_.line;
    position_.column = 1;
    previous_was_carriage_return_ = false;
  } else {
    ++position_.column;
    previous_was_carriage_return_ = false;
  }
}

void Lexer::advance(const std::size_t count) noexcept {
  for (std::size_t index = 0; index < count; ++index)
    advance();
}

void Lexer::report(const Position start, std::string message) {
  diagnostics_.push_back({.severity = core::Severity::Error,
                          .span = make_span(start),
                          .message = std::move(message)});
}

bool Lexer::starts_with(const std::string_view text) const noexcept {
  return std::string_view{source_}.substr(position_.offset).starts_with(text);
}

char Lexer::peek(const std::size_t lookahead) const noexcept {
  const std::size_t index = position_.offset + lookahead;
  return index < source_.size() ? source_[index] : '\0';
}

bool Lexer::is_identifier_start(const char c) noexcept {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

bool Lexer::is_identifier_continue(const char c) noexcept {
  return is_identifier_start(c) || is_ascii_digit(c) || c == '-';
}

bool Lexer::is_ascii_digit(const char c) noexcept {
  return c >= '0' && c <= '9';
}

bool Lexer::is_hex_digit(const char c) noexcept {
  return is_ascii_digit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

std::string_view to_string(const TokenKind kind) noexcept {
  switch (kind) {
  case TokenKind::Integer:
    return "Integer";
  case TokenKind::Float:
    return "Float";
  case TokenKind::Length:
    return "Length";
  case TokenKind::Color:
    return "Color";
  case TokenKind::String:
    return "String";
  case TokenKind::True:
    return "True";
  case TokenKind::False:
    return "False";
  case TokenKind::Ident:
    return "Ident";
  case TokenKind::LBrace:
    return "LBrace";
  case TokenKind::RBrace:
    return "RBrace";
  case TokenKind::Colon:
    return "Colon";
  case TokenKind::Eof:
    return "Eof";
  case TokenKind::Error:
    return "Error";
  }
  return "Unknown";
}

} // namespace sampan::lex
