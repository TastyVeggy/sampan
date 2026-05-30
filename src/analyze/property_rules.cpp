#include "property_rules.hpp"

namespace sampan::analyze::detail {
namespace {

[[nodiscard]] bool is_universal(const node::PropertyId property) noexcept {
  switch (property) {
  case node::PropertyId::Width:
  case node::PropertyId::Height:
  case node::PropertyId::Padding:
  case node::PropertyId::PaddingTop:
  case node::PropertyId::PaddingRight:
  case node::PropertyId::PaddingBottom:
  case node::PropertyId::PaddingLeft:
  case node::PropertyId::Margin:
  case node::PropertyId::MarginTop:
  case node::PropertyId::MarginRight:
  case node::PropertyId::MarginBottom:
  case node::PropertyId::MarginLeft:
  case node::PropertyId::BorderWidth:
  case node::PropertyId::BorderColor:
  case node::PropertyId::Background:
    return true;
  case node::PropertyId::Title:
  case node::PropertyId::Gap:
  case node::PropertyId::Align:
  case node::PropertyId::Content:
  case node::PropertyId::Color:
  case node::PropertyId::Size:
  case node::PropertyId::Weight:
  case node::PropertyId::Level:
  case node::PropertyId::Count:
    return false;
  }
  return false;
}

} // namespace

bool property_allowed(const node::NodeKind node_kind,
                      const node::PropertyId property) noexcept {
  if (is_universal(property)) {
    return true;
  }

  switch (node_kind) {
  case node::NodeKind::Page:
    return property == node::PropertyId::Title;
  case node::NodeKind::Stack:
    return property == node::PropertyId::Gap;
  case node::NodeKind::Row:
    return property == node::PropertyId::Gap ||
           property == node::PropertyId::Align;
  case node::NodeKind::Text:
    return property == node::PropertyId::Content ||
           property == node::PropertyId::Color ||
           property == node::PropertyId::Size ||
           property == node::PropertyId::Weight;
  case node::NodeKind::Heading:
    return property == node::PropertyId::Content ||
           property == node::PropertyId::Color ||
           property == node::PropertyId::Size ||
           property == node::PropertyId::Level;
  case node::NodeKind::Spacer:
    return property == node::PropertyId::Size;
  case node::NodeKind::Box:
  case node::NodeKind::Button:
    return false;
  }
  return false;
}

} // namespace sampan::analyze::detail
