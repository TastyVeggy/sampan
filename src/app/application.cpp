#include "sampan/app/application.hpp"

#include <QApplication>
#include <QFileInfo>
#include <QString>

#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "sampan/analyze/analyzer.hpp"
#include "sampan/app/document_view.hpp"
#include "sampan/parse/parser.hpp"

namespace sampan::app {
namespace {

constexpr int kUserProgramError = 3;
constexpr int kIoError = 5;

void print_diagnostics(const char *source_path,
                       const std::vector<core::Diagnostic> &diagnostics) {
  for (const core::Diagnostic &diagnostic : diagnostics) {
    std::cerr << source_path << ":" << diagnostic.span.start_line << ":"
              << diagnostic.span.start_column << ": "
              << core::to_string(diagnostic.severity) << ": "
              << diagnostic.message << "\n";
  }
}

} // namespace

int run(int argc, char *argv[], const char *source_path) {
  std::ifstream input{source_path, std::ios::binary};
  if (!input) {
    std::cerr << "error: cannot open " << source_path << "\n";
    return kIoError;
  }

  std::string source{std::istreambuf_iterator<char>{input},
                     std::istreambuf_iterator<char>{}};
  parse::ParseResult parsed = parse::parse(std::move(source), 0);
  if (!parsed.diagnostics.empty() || parsed.document == nullptr) {
    print_diagnostics(source_path, parsed.diagnostics);
    return kUserProgramError;
  }

  analyze::AnalyzeResult analyzed = analyze::analyze(*parsed.document);
  if (!analyzed.diagnostics.empty() || analyzed.tree == nullptr) {
    print_diagnostics(source_path, analyzed.diagnostics);
    return kUserProgramError;
  }

  QApplication application{argc, argv};
  DocumentView view{std::move(analyzed.tree)};
  view.setWindowTitle(QFileInfo{QString::fromUtf8(source_path)}.fileName() +
                      " — Sampan");
  view.show();
  return application.exec();
}

} // namespace sampan::app
