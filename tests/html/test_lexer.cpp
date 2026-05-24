#include <gtest/gtest.h>

#include <sampan/html/lexer.hpp>

using namespace sampan::html;

TEST(Lexer, NonEmptyInputIsNotDone) {
  std::string dummy_html = "test";

  Lexer lexer{dummy_html};
  EXPECT_EQ(lexer.done(), false);
}

TEST(Lexer, EmptyInputIsDone) {
  std::string dummy_html = "";

  Lexer lexer{dummy_html};
  EXPECT_EQ(lexer.done(), true);
}
