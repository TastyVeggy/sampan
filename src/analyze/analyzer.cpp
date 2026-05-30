#include "sampan/analyze/analyzer.hpp"

#include <array>
#include <optional>
#include <string_view>
#include <utility>

#include "sampan/core/overloaded.hpp"

#include "builtins.hpp"
#include "child_rules.hpp"
#include "property_rules.hpp"

namespace sampan::analyze {
namespace {

constexpr std::size_t kPropertyCount =
    static_cast<std::size_t>(node::PropertyId::Count);
using SeenProperties = std::array<bool, kPropertyCount>;

[[nodiscard]] detail::ValueType
literal_type(const ast::Literal &literal) noexcept {
  return std::visit(
      core::Overloaded{
          [](const ast::IntegerLiteral &) {
            return detail::ValueType::Integer;
          },
          [](const ast::FloatLiteral &) { return detail::ValueType::Float; },
          [](const ast::LengthLiteral &) { return detail::ValueType::Length; },
          [](const ast::ColorLiteral &) { return detail::ValueType::Color; },
          [](const ast::StringLiteral &) { return detail::ValueType::String; },
          [](const ast::BooleanLiteral &) {
            return detail::ValueType::Boolean;
          }},
      literal);
}

[[nodiscard]] node::Value resolve_value(const ast::Literal &literal) {
  return std::visit(
      core::Overloaded{[](const ast::IntegerLiteral &value) -> node::Value {
                         return value.value;
                       },
                       [](const ast::FloatLiteral &value) -> node::Value {
                         return value.value;
                       },
                       [](const ast::LengthLiteral &value) -> node::Value {
                         return node::Length{.pixels = value.pixels};
                       },
                       [](const ast::ColorLiteral &value) -> node::Value {
                         return node::Color{.red = value.red,
                                            .green = value.green,
                                            .blue = value.blue};
                       },
                       [](const ast::StringLiteral &value) -> node::Value {
                         return value.value;
                       },
                       [](const ast::BooleanLiteral &value) -> node::Value {
                         return value.value;
                       }},
      literal);
}

class Analyzer {
public:
  [[nodiscard]] AnalyzeResult run(const ast::Document &document) {
    if (document.root == nullptr) {
      report(document.span, "document has no root node");
      return {.tree = nullptr, .diagnostics = std::move(diagnostics_)};
    }

    auto tree = std::make_unique<node::Tree>();
    tree->root = analyze_node(*document.root, true);
    if (!diagnostics_.empty()) {
      tree.reset();
    }

    return {.tree = std::move(tree), .diagnostics = std::move(diagnostics_)};
  }

private:
  [[nodiscard]] std::unique_ptr<node::Node>
  analyze_node(const ast::Node &source, const bool is_root) {
    const std::optional<node::NodeKind> kind =
        detail::resolve_node_kind(source.tag);
    if (!kind.has_value()) {
      report(source.span, "unknown node type '" + source.tag + "'");
      return nullptr;
    }

    if (is_root && *kind != node::NodeKind::Page) {
      report(source.span, "document root must be a 'page' node");
    } else if (!is_root && *kind == node::NodeKind::Page) {
      report(source.span, "node 'page' is only allowed at document root");
    }

    auto resolved = std::make_unique<node::Node>();
    resolved->kind = *kind;
    resolved->span = source.span;
    SeenProperties seen{};

    for (const ast::Item &item : source.items) {
      std::visit(
          core::Overloaded{
              [this, kind, &seen, &resolved](const ast::Property &property) {
                analyze_property(property, *kind, seen, *resolved);
              },
              [&resolved](const ast::TextChild &text) {
                resolved->items.emplace_back(node::TextChild{
                    .value = text.value.value, .span = text.span});
              },
              [this, kind, &resolved](const ast::ChildNode &child) {
                std::unique_ptr<node::Node> resolved_child =
                    analyze_node(*child.value, false);
                if (resolved_child == nullptr) {
                  return;
                }

                if (!detail::child_allowed(*kind, resolved_child->kind)) {
                  report(
                      child.value->span,
                      "node '" + std::string{node::to_string(*kind)} +
                          "' cannot contain child node '" +
                          std::string{node::to_string(resolved_child->kind)} +
                          "'");
                  return;
                }

                resolved->items.emplace_back(
                    node::ChildNode{.value = std::move(resolved_child)});
              }},
          item);
    }

    return resolved;
  }

  void analyze_property(const ast::Property &source,
                        const node::NodeKind node_kind, SeenProperties &seen,
                        node::Node &resolved) {
    const std::optional<detail::PropertyDefinition> definition =
        detail::resolve_property(source.name);
    if (!definition.has_value()) {
      report(source.span, "unknown property '" + source.name + "' on node '" +
                              std::string{node::to_string(node_kind)} + "'");
      return;
    }

    if (!detail::property_allowed(node_kind, definition->id)) {
      report(source.span, "property '" + source.name +
                              "' is not valid on node '" +
                              std::string{node::to_string(node_kind)} + "'");
      return;
    }

    const std::size_t property_index = static_cast<std::size_t>(definition->id);
    if (seen[property_index]) {
      report(source.span, "duplicate property '" + source.name + "' on node '" +
                              std::string{node::to_string(node_kind)} + "'");
      return;
    }
    seen[property_index] = true;

    const detail::ValueType actual_type = literal_type(source.value);
    if (actual_type != definition->type) {
      report(source.span,
             "property '" + source.name + "' on node '" +
                 std::string{node::to_string(node_kind)} + "' expects " +
                 std::string{detail::to_string(definition->type)} + ", got " +
                 std::string{detail::to_string(actual_type)});
      return;
    }

    resolved.items.emplace_back(
        node::Property{.id = definition->id,
                       .value = resolve_value(source.value),
                       .span = source.span});
  }

  void report(const core::SourceSpan span, std::string message) {
    diagnostics_.push_back({.severity = core::Severity::Error,
                            .span = span,
                            .message = std::move(message)});
  }

  std::vector<core::Diagnostic> diagnostics_;
};

} // namespace

AnalyzeResult analyze(const ast::Document &document) {
  return Analyzer{}.run(document);
}

} // namespace sampan::analyze
