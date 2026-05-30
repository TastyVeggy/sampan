#include "child_rules.hpp"

namespace sampan::analyze::detail {

bool child_allowed(const node::NodeKind parent,
                   const node::NodeKind /* child */) noexcept {
  switch (parent) {
  case node::NodeKind::Page:
  case node::NodeKind::Stack:
  case node::NodeKind::Row:
  case node::NodeKind::Box:
    return true;
  case node::NodeKind::Text:
  case node::NodeKind::Heading:
  case node::NodeKind::Button:
  case node::NodeKind::Spacer:
    return false;
  }
  return false;
}

} // namespace sampan::analyze::detail
