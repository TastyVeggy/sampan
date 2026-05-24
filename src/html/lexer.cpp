#include <sampan/html/lexer.hpp>

namespace sampan::html {
Lexer::Lexer(std::string source) : src_(std::move(source)), pos_(0) {};

Token Lexer::next() {
  pos_++;
  return {.kind = TokenKind::Eof, .data = ""};
}

bool Lexer::done() const { return pos_ >= src_.size(); }

}  // namespace sampan::html
