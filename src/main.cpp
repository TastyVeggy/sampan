#include <charconv>
#include <cmath>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "sampan/analyze/analyzer.hpp"
#include "sampan/app/application.hpp"
#include "sampan/ast/dump.hpp"
#include "sampan/layout/dump.hpp"
#include "sampan/layout/layout.hpp"
#include "sampan/lex/lexer.hpp"
#include "sampan/node/dump.hpp"
#include "sampan/parse/parser.hpp"

namespace {

constexpr int kSuccess = 0;
constexpr int kCliMisuse = 2;
constexpr int kUserProgramError = 3;
constexpr int kIoError = 5;

void print_usage(std::ostream &output) {
  output << "Usage:\n"
         << "  sampan [path-or-url] Open the Sampan browser\n"
         << "  sampan <command> [arguments]\n\n"
         << "Commands:\n"
         << "  version    Print the Sampan version\n"
         << "  lex <path> Dump Yuloh tokens\n"
         << "  parse <path> Dump the Yuloh AST\n"
         << "  analyze <path> Dump the validated Yuloh node tree\n"
         << "  layout <path> <width> <height> Dump the layout tree\n";
}

[[nodiscard]] std::optional<double>
parse_viewport_dimension(const std::string_view text) {
  double value = 0.0;
  const char *const end = text.data() + text.size();
  const auto [pointer, error] = std::from_chars(text.data(), end, value);
  if (error != std::errc{} || pointer != end || !std::isfinite(value) ||
      value <= 0.0) {
    return std::nullopt;
  }
  return value;
}

} // namespace

int main(const int argc, char *argv[]) {
  if (argc == 2 && std::string_view{argv[1]} == "version") {
    std::cout << "Sampan 0.1.0 \n";
    return kSuccess;
  }
  if (argc == 3 && std::string_view{argv[1]} == "lex") {
    std::ifstream input{argv[2], std::ios::binary};
    if (!input) {
      std::cerr << "error: cannot open " << argv[2] << "\n";
      return kIoError;
    }
    std::string source{std::istreambuf_iterator<char>{input},
                       std::istreambuf_iterator<char>{}};
    sampan::lex::Lexer lexer{std::move(source), 0};
    for (;;) {
      const sampan::lex::Token token = lexer.next();
      std::cout << sampan::lex::to_string(token.kind);
      if (!token.text.empty()) {
        std::cout << " " << token.text;
      }
      std::cout << "\n";
      if (token.kind == sampan::lex::TokenKind::Eof) {
        break;
      }
    }
    const std::vector<sampan::core::Diagnostic> diagnostics =
        std::move(lexer).diagnostics();
    for (const sampan::core::Diagnostic &diagnostic : diagnostics) {
      std::cerr << argv[2] << ":" << diagnostic.span.start_line << ":"
                << diagnostic.span.start_column << ": "
                << sampan::core::to_string(diagnostic.severity) << ": "
                << diagnostic.message << "\n";
    }
    return diagnostics.empty() ? kSuccess : kUserProgramError;
  }
  if (argc == 3 && std::string_view{argv[1]} == "parse") {
    std::ifstream input{argv[2], std::ios::binary};
    if (!input) {
      std::cerr << "error: cannot open " << argv[2] << "\n";
      return kIoError;
    }
    std::string source{std::istreambuf_iterator<char>{input},
                       std::istreambuf_iterator<char>{}};
    sampan::parse::ParseResult result =
        sampan::parse::parse(std::move(source), 0);
    if (result.document != nullptr) {
      std::cout << sampan::ast::dump(*result.document);
    }
    for (const sampan::core::Diagnostic &diagnostic : result.diagnostics) {
      std::cerr << argv[2] << ":" << diagnostic.span.start_line << ":"
                << diagnostic.span.start_column << ": "
                << sampan::core::to_string(diagnostic.severity) << ": "
                << diagnostic.message << "\n";
    }
    return result.diagnostics.empty() ? kSuccess : kUserProgramError;
  }
  if (argc == 3 && std::string_view{argv[1]} == "analyze") {
    std::ifstream input{argv[2], std::ios::binary};
    if (!input) {
      std::cerr << "error: cannot open " << argv[2] << "\n";
      return kIoError;
    }
    std::string source{std::istreambuf_iterator<char>{input},
                       std::istreambuf_iterator<char>{}};
    sampan::parse::ParseResult parsed =
        sampan::parse::parse(std::move(source), 0);
    if (!parsed.diagnostics.empty() || parsed.document == nullptr) {
      for (const sampan::core::Diagnostic &diagnostic : parsed.diagnostics) {
        std::cerr << argv[2] << ":" << diagnostic.span.start_line << ":"
                  << diagnostic.span.start_column << ": "
                  << sampan::core::to_string(diagnostic.severity) << ": "
                  << diagnostic.message << "\n";
      }
      return kUserProgramError;
    }

    const sampan::analyze::AnalyzeResult analyzed =
        sampan::analyze::analyze(*parsed.document);
    if (analyzed.tree != nullptr) {
      std::cout << sampan::node::dump(*analyzed.tree);
    }
    for (const sampan::core::Diagnostic &diagnostic : analyzed.diagnostics) {
      std::cerr << argv[2] << ":" << diagnostic.span.start_line << ":"
                << diagnostic.span.start_column << ": "
                << sampan::core::to_string(diagnostic.severity) << ": "
                << diagnostic.message << "\n";
    }
    return analyzed.diagnostics.empty() ? kSuccess : kUserProgramError;
  }
  if (argc == 5 && std::string_view{argv[1]} == "layout") {
    const std::optional<double> width = parse_viewport_dimension(argv[3]);
    const std::optional<double> height = parse_viewport_dimension(argv[4]);
    if (!width.has_value() || !height.has_value()) {
      std::cerr
          << "error: viewport width and height must be positive numbers\n";
      return kCliMisuse;
    }

    std::ifstream input{argv[2], std::ios::binary};
    if (!input) {
      std::cerr << "error: cannot open " << argv[2] << "\n";
      return kIoError;
    }
    std::string source{std::istreambuf_iterator<char>{input},
                       std::istreambuf_iterator<char>{}};
    sampan::parse::ParseResult parsed =
        sampan::parse::parse(std::move(source), 0);
    if (!parsed.diagnostics.empty() || parsed.document == nullptr) {
      for (const sampan::core::Diagnostic &diagnostic : parsed.diagnostics) {
        std::cerr << argv[2] << ":" << diagnostic.span.start_line << ":"
                  << diagnostic.span.start_column << ": "
                  << sampan::core::to_string(diagnostic.severity) << ": "
                  << diagnostic.message << "\n";
      }
      return kUserProgramError;
    }

    const sampan::analyze::AnalyzeResult analyzed =
        sampan::analyze::analyze(*parsed.document);
    if (!analyzed.diagnostics.empty() || analyzed.tree == nullptr) {
      for (const sampan::core::Diagnostic &diagnostic : analyzed.diagnostics) {
        std::cerr << argv[2] << ":" << diagnostic.span.start_line << ":"
                  << diagnostic.span.start_column << ": "
                  << sampan::core::to_string(diagnostic.severity) << ": "
                  << diagnostic.message << "\n";
      }
      return kUserProgramError;
    }

    const std::unique_ptr<sampan::layout::LayoutBox> root =
        sampan::layout::build(
            *analyzed.tree,
            {.x = 0.0, .y = 0.0, .width = *width, .height = *height});
    if (root != nullptr) {
      std::cout << sampan::layout::dump(*root);
    }
    return kSuccess;
  }
  if (argc == 1) {
    return sampan::app::run(argc, argv, std::nullopt);
  }
  if (argc == 2) {
    const std::string_view argument{argv[1]};
    if (argument == "lex" || argument == "parse" || argument == "analyze" ||
        argument == "layout") {
      print_usage(std::cerr);
      return kCliMisuse;
    }
    return sampan::app::run(argc, argv, std::string{argument});
  }
  print_usage(std::cerr);
  return kCliMisuse;
}
