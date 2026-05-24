#include <string>

#include "sampan/html/lexer.hpp"

int main() {
  std::string dummy_html = "<div>Hello World!</div>";

  sampan::html::Lexer lexer{dummy_html};
  while (!lexer.done()) {
    lexer.next();
  }
}
