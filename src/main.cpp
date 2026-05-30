#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "sampan/analyze/analyzer.hpp"
#include "sampan/ast/dump.hpp"
#include "sampan/lex/lexer.hpp"
#include "sampan/node/dump.hpp"
#include "sampan/parse/parser.hpp"

namespace {

constexpr int kSuccess = 0;
constexpr int kCliMisuse = 2;
constexpr int kUserProgramError = 3;
constexpr int kIoError = 5;

void print_usage(std::ostream &output) {
  output << "Usage: sampan <command>\nCommands:\n"
         << "  version    Print the Sampan version\n"
         << "  lex <path> Dump Yuloh tokens\n"
         << "  parse <path> Dump the Yuloh AST\n"
         << "  analyze <path> Dump the validated Yuloh node tree\n";
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
  print_usage(argc == 1 ? std::cout : std::cerr);
  return argc == 1 ? kSuccess : kCliMisuse;
}
