#pragma once
#include <string>
#include <string_view>

namespace sampan::html {
enum class TokenKind { DocType, StartTag, EndTag, Text, Comment, Eof };

struct Token {
  TokenKind kind;
  std::string_view data;
};

class Lexer {
 public:
  explicit Lexer(std::string source);

  Token next();
  bool done() const;

 private:
  std::string src_;
  size_t pos_{0};
};

}  // namespace sampan::html
