#include "sampan/parse/parser.hpp"

#include <algorithm>
#include <charconv>
#include <iterator>
#include <optional>
#include <string_view>
#include <system_error>
#include <utility>

#include "sampan/lex/lexer.hpp"

namespace sampan::parse {
namespace {

using lex::Token;
using lex::TokenKind;

class Parser {
public:
  Parser(std::string &&source, const std::uint32_t file_id)
      : lexer_(std::move(source), file_id) {
    current_ = read_token();
    next_ = read_token();
  }

  [[nodiscard]] ParseResult run() {
    auto document = std::make_unique<ast::Document>();
    document->root = parse_node();
    if (document->root != nullptr) {
      document->span = document->root->span;
    }

    if (document->root != nullptr && current_.kind != TokenKind::Eof) {
      report(current_.span, "expected end of document after root node");
    }

    std::vector<core::Diagnostic> lexical_diagnostics =
        std::move(lexer_).diagnostics();
    diagnostics_.insert(diagnostics_.end(),
                        std::make_move_iterator(lexical_diagnostics.begin()),
                        std::make_move_iterator(lexical_diagnostics.end()));
    std::stable_sort(
        diagnostics_.begin(), diagnostics_.end(),
        [](const core::Diagnostic &left, const core::Diagnostic &right) {
          return left.span.start_offset < right.span.start_offset;
        });

    if (document->root == nullptr) {
      document.reset();
    }
    return {.document = std::move(document),
            .diagnostics = std::move(diagnostics_)};
  }

private:
  [[nodiscard]] std::unique_ptr<ast::Node> parse_node() {
    if (current_.kind != TokenKind::Ident) {
      report(current_.span, "expected node name");
      return nullptr;
    }

    const Token name = take();
    if (current_.kind != TokenKind::LBrace) {
      report(current_.span,
             "expected '{' after node name '" + std::string{name.text} + "'");
      return nullptr;
    }

    advance();
    return parse_node_body(name);
  }

  [[nodiscard]] std::unique_ptr<ast::Node> parse_node_body(const Token &name) {
    auto node = std::make_unique<ast::Node>();
    node->tag = name.text;

    while (current_.kind != TokenKind::RBrace &&
           current_.kind != TokenKind::Eof) {
      std::optional<ast::Item> item = parse_item();
      if (item.has_value()) {
        node->items.emplace_back(std::move(*item));
      } else {
        recover_item();
      }
    }

    if (current_.kind == TokenKind::Eof) {
      report(current_.span,
             "expected '}' to close node '" + std::string{name.text} + "'");
      node->span = join_spans(name.span, current_.span);
      return node;
    }

    node->span = join_spans(name.span, current_.span);
    advance();
    return node;
  }

  [[nodiscard]] std::optional<ast::Item> parse_item() {
    if (current_.kind == TokenKind::String) {
      const ast::StringLiteral literal = string_literal(take());
      return ast::Item{ast::TextChild{.value = literal, .span = literal.span}};
    }

    if (current_.kind != TokenKind::Ident) {
      report(current_.span, "expected a property, text child, or child node");
      return std::nullopt;
    }

    if (next_.kind == TokenKind::Colon) {
      std::optional<ast::Property> property = parse_property();
      if (property.has_value()) {
        return ast::Item{std::move(*property)};
      }
      return std::nullopt;
    }

    if (next_.kind == TokenKind::LBrace) {
      std::unique_ptr<ast::Node> child = parse_node();
      if (child != nullptr) {
        return ast::Item{ast::ChildNode{.value = std::move(child)}};
      }
      return std::nullopt;
    }

    const Token name = take();
    report(current_.span, "expected ':' or '{' after identifier '" +
                              std::string{name.text} + "'");
    return std::nullopt;
  }

  [[nodiscard]] std::optional<ast::Property> parse_property() {
    const Token name = take();
    advance();

    if (!is_literal(current_.kind)) {
      report(current_.span, "expected a literal property value");
      return std::nullopt;
    }

    const Token value = take();
    return ast::Property{.name = std::string{name.text},
                         .value = make_literal(value),
                         .span = join_spans(name.span, value.span)};
  }

  [[nodiscard]] static bool is_literal(const TokenKind kind) noexcept {
    return kind == TokenKind::Integer || kind == TokenKind::Float ||
           kind == TokenKind::Length || kind == TokenKind::Color ||
           kind == TokenKind::String || kind == TokenKind::True ||
           kind == TokenKind::False;
  }

  [[nodiscard]] static ast::Literal make_literal(const Token &token) {
    switch (token.kind) {
    case TokenKind::Integer:
      return ast::IntegerLiteral{.value = parse_integer(token.text),
                                 .span = token.span};
    case TokenKind::Float:
      return ast::FloatLiteral{.value = parse_float(token.text),
                               .span = token.span};
    case TokenKind::Length:
      return ast::LengthLiteral{
          .pixels = parse_float(token.text.substr(0, token.text.size() - 2)),
          .span = token.span};
    case TokenKind::Color:
      return color_literal(token);
    case TokenKind::String:
      return string_literal(token);
    case TokenKind::True:
      return ast::BooleanLiteral{.value = true, .span = token.span};
    case TokenKind::False:
      return ast::BooleanLiteral{.value = false, .span = token.span};
    default:
      return ast::StringLiteral{.value = "", .span = token.span};
    }
  }

  [[nodiscard]] static std::int64_t parse_integer(const std::string_view text) {
    std::int64_t value = 0;
    const auto [pointer, error] =
        std::from_chars(text.data(), text.data() + text.size(), value);
    (void)pointer;
    return error == std::errc{} ? value : 0;
  }

  [[nodiscard]] static double parse_float(const std::string_view text) {
    double value = 0;
    const auto [pointer, error] =
        std::from_chars(text.data(), text.data() + text.size(), value);
    (void)pointer;
    return error == std::errc{} ? value : 0;
  }

  [[nodiscard]] static ast::ColorLiteral color_literal(const Token &token) {
    const std::string_view digits = token.text.substr(1);
    const auto component = [](const char first, const char second) {
      const auto hex_value = [](const char character) -> std::uint8_t {
        if (character >= '0' && character <= '9') {
          return static_cast<std::uint8_t>(character - '0');
        }
        if (character >= 'a' && character <= 'f') {
          return static_cast<std::uint8_t>(character - 'a' + 10);
        }
        return static_cast<std::uint8_t>(character - 'A' + 10);
      };
      return static_cast<std::uint8_t>((hex_value(first) << 4) |
                                       hex_value(second));
    };

    if (digits.size() == 3) {
      return {.red = component(digits[0], digits[0]),
              .green = component(digits[1], digits[1]),
              .blue = component(digits[2], digits[2]),
              .span = token.span};
    }
    return {.red = component(digits[0], digits[1]),
            .green = component(digits[2], digits[3]),
            .blue = component(digits[4], digits[5]),
            .span = token.span};
  }

  [[nodiscard]] static ast::StringLiteral string_literal(const Token &token) {
    std::string value;
    const std::string_view text = token.text.substr(1, token.text.size() - 2);
    value.reserve(text.size());
    for (std::size_t index = 0; index < text.size(); ++index) {
      if (text[index] != '\\') {
        value += text[index];
        continue;
      }
      ++index;
      switch (text[index]) {
      case 'n':
        value += '\n';
        break;
      case 't':
        value += '\t';
        break;
      default:
        value += text[index];
        break;
      }
    }
    return {.value = std::move(value), .span = token.span};
  }

  [[nodiscard]] static core::SourceSpan
  join_spans(const core::SourceSpan &first,
             const core::SourceSpan &last) noexcept {
    return {.file_id = first.file_id,
            .start_offset = first.start_offset,
            .end_offset = last.end_offset,
            .start_line = first.start_line,
            .start_column = first.start_column,
            .end_line = last.end_line,
            .end_column = last.end_column};
  }

  void recover_item() {
    while (current_.kind != TokenKind::RBrace &&
           current_.kind != TokenKind::Eof) {
      if (current_.kind == TokenKind::Ident ||
          current_.kind == TokenKind::String) {
        return;
      }
      advance();
    }
  }

  [[nodiscard]] Token take() {
    const Token token = current_;
    advance();
    return token;
  }

  [[nodiscard]] Token read_token() {
    Token token;
    do {
      token = lexer_.next();
    } while (token.kind == TokenKind::Error);
    return token;
  }

  void advance() {
    current_ = next_;
    next_ = read_token();
  }

  void report(const core::SourceSpan span, std::string message) {
    diagnostics_.push_back({.severity = core::Severity::Error,
                            .span = span,
                            .message = std::move(message)});
  }

  lex::Lexer lexer_;
  Token current_{};
  Token next_{};
  std::vector<core::Diagnostic> diagnostics_;
};

} // namespace

ParseResult parse(std::string &&source, const std::uint32_t file_id) {
  return Parser{std::move(source), file_id}.run();
}

} // namespace sampan::parse
