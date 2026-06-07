#include "property_values.hpp"

#include <array>
#include <sstream>
#include <string_view>
#include <utility>

namespace sampan::analyze::detail {
namespace {

template <typename T> struct NamedValue {
  std::string_view name;
  T value;
};

constexpr std::array<NamedValue<node::Alignment>, 3> kAlignments{{
    {"start", node::Alignment::Start},
    {"center", node::Alignment::Center},
    {"end", node::Alignment::End},
}};

constexpr std::array<NamedValue<node::Justification>, 3> kJustifications{{
    {"start", node::Justification::Start},
    {"center", node::Justification::Center},
    {"end", node::Justification::End},
}};

template <typename T, std::size_t Size>
[[nodiscard]] std::string
expected_values_message(const std::array<NamedValue<T>, Size> &values) {
  std::ostringstream message;
  message << "expects one of ";
  for (std::size_t index = 0; index < values.size(); ++index) {
    if (index != 0) {
      message << (index + 1 == values.size() ? ", or " : ", ");
    }
    message << "'" << values[index].name << "'";
  }
  return message.str();
}

template <typename T, std::size_t Size>
[[nodiscard]] std::expected<node::Value, std::string>
normalize_named_value(node::Value value,
                      const std::array<NamedValue<T>, Size> &values) {
  const auto *text = std::get_if<std::string>(&value);
  if (text != nullptr) {
    for (const NamedValue<T> &named_value : values) {
      if (*text == named_value.name) {
        return named_value.value;
      }
    }
  }
  return std::unexpected{expected_values_message(values)};
}

} // namespace

std::expected<node::Value, std::string> normalize_alignment(node::Value value) {
  return normalize_named_value(std::move(value), kAlignments);
}

std::expected<node::Value, std::string>
normalize_justification(node::Value value) {
  return normalize_named_value(std::move(value), kJustifications);
}

} // namespace sampan::analyze::detail
