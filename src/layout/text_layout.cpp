#include "text_layout.hpp"

#include <algorithm>
#include <cstddef>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace sampan::layout {
namespace {

[[nodiscard]] std::size_t code_point_size(const std::string_view text,
                                          const std::size_t offset) noexcept {
  const auto first = static_cast<unsigned char>(text[offset]);
  if ((first & 0x80U) == 0U) {
    return 1;
  }

  std::size_t size = 1;
  if ((first & 0xE0U) == 0xC0U) { // first three bits should be 110
    size = 2;
  } else if ((first & 0xF0U) == 0xE0U) { // first four bits should be 1110
    size = 3;
  } else if ((first & 0xF8U) == 0xF0U) { // first 5 bits should be 1111 0
    size = 4;
  }

  if (offset + size > text.size()) {
    return 1;
  }
  for (std::size_t index = 1; index < size; ++index) {
    const auto continuation = static_cast<unsigned char>(text[offset + index]);
    if ((continuation & 0xC0U) != 0x80U) {
      return 1;
    }
  }
  return size;
}

[[nodiscard]] std::size_t
code_point_count(const std::string_view text) noexcept {
  std::size_t count = 0;
  for (std::size_t offset = 0; offset < text.size();) {
    offset += code_point_size(text, offset);
    ++count;
  }
  return count;
}

// fallback
class FixedTextMetrics final : public TextMetrics {
public:
  [[nodiscard]] double width(const std::string_view text,
                             const TextStyle &style) const override {
    return static_cast<double>(code_point_count(text)) * style.font_size * 0.55;
  }

  [[nodiscard]] double line_height(const TextStyle &style) const override {
    return style.font_size * 1.2;
  }
};

} // namespace

const TextMetrics &fixed_text_metrics() noexcept {
  static const FixedTextMetrics metrics;
  return metrics;
}

namespace detail {
namespace {

[[nodiscard]] std::size_t next_code_point(const std::string_view text,
                                          const std::size_t offset) noexcept {
  return offset + code_point_size(text, offset);
}

[[nodiscard]] bool is_collapsible_space(const char character) noexcept {
  return character == ' ' || character == '\t' || character == '\r' ||
         character == '\f' || character == '\v';
}

void append_line(std::vector<WrappedLine> &lines, std::string text,
                 const TextStyle &style, const TextMetrics &metrics) {
  const double width = metrics.width(text, style);
  lines.push_back({.text = std::move(text), .width = width});
}

void split_word(std::vector<WrappedLine> &lines, std::string_view word,
                const double maximum_width, const TextStyle &style,
                const TextMetrics &metrics, std::string &line) {
  std::vector<std::size_t> boundaries{0};
  while (boundaries.back() < word.size()) {
    boundaries.push_back(next_code_point(word, boundaries.back()));
  }

  std::size_t first = 0;
  // if first character is already too big, then too bad, we
  // just put it
  while (first + 1 < boundaries.size()) {
    std::size_t fitting = first + 1;
    std::size_t low = first + 1;
    std::size_t high = boundaries.size() - 1;
    while (low <= high) {
      const std::size_t middle = low + (high - low) / 2;
      const std::string_view candidate = word.substr(
          boundaries[first], boundaries[middle] - boundaries[first]);
      if (metrics.width(candidate, style) <= maximum_width ||
          middle == first + 1) {
        fitting = middle;
        low = middle + 1;
      } else {
        high = middle - 1;
      }
    }

    const std::string_view fragment =
        word.substr(boundaries[first], boundaries[fitting] - boundaries[first]);
    if (fitting + 1 == boundaries.size()) {
      line.assign(fragment);
      return;
    }
    append_line(lines, std::string{fragment}, style, metrics);
    first = fitting;
  }
}

void append_word(std::vector<WrappedLine> &lines, std::string &line,
                 const std::string_view word, const double maximum_width,
                 const TextStyle &style, const TextMetrics &metrics) {
  std::string candidate = line;
  if (!candidate.empty()) {
    candidate.push_back(' ');
  }
  candidate.append(word);

  // if current line + space + new word fit, then keep on current line
  if (metrics.width(candidate, style) <= maximum_width) {
    line = std::move(candidate);
    return;
  }

  // otherwise we finish the current line
  if (!line.empty()) {
    append_line(lines, std::move(line), style, metrics);
    line.clear();
  }

  // see if word fit by itself on the fresh line
  if (metrics.width(word, style) <= maximum_width) {
    line.assign(word);
    return;
  }

  // word by itself too long, then we need to split it
  split_word(lines, word, maximum_width, style, metrics, line);
}

void wrap_paragraph(std::vector<WrappedLine> &lines,
                    const std::string_view paragraph,
                    const double maximum_width, const TextStyle &style,
                    const TextMetrics &metrics) {
  const std::size_t first_line = lines.size();
  std::string line;
  std::size_t offset = 0;
  while (offset < paragraph.size()) {
    while (offset < paragraph.size() &&
           is_collapsible_space(paragraph[offset])) {
      ++offset;
    }
    if (offset == paragraph.size()) {
      break;
    }

    const std::size_t word_begin = offset;
    while (offset < paragraph.size() &&
           !is_collapsible_space(paragraph[offset])) {
      offset = next_code_point(paragraph, offset);
    }
    append_word(lines, line, paragraph.substr(word_begin, offset - word_begin),
                maximum_width, style, metrics);
  }

  if (!line.empty() || lines.size() == first_line) {
    append_line(lines, std::move(line), style, metrics);
  }
}

} // namespace

double unwrapped_width(const std::string_view text, const TextStyle &style,
                       const TextMetrics &metrics) {
  double width = 0.0;
  std::size_t line_begin = 0;
  while (line_begin <= text.size()) {
    const std::size_t line_end = text.find('\n', line_begin);
    const std::string_view line =
        line_end == std::string_view::npos
            ? text.substr(line_begin)
            : text.substr(line_begin, line_end - line_begin);
    width = std::max(width, metrics.width(line, style));
    if (line_end == std::string_view::npos) {
      break;
    }
    line_begin = line_end + 1;
  }
  return width;
}

WrappedText wrap_text(const std::string_view text, const TextStyle &style,
                      const double maximum_width, const TextMetrics &metrics) {
  const double available_width = std::max(0.0, maximum_width);
  std::vector<WrappedLine> lines;
  std::size_t paragraph_begin = 0;
  while (paragraph_begin <= text.size()) {
    const std::size_t paragraph_end = text.find('\n', paragraph_begin);
    const std::string_view paragraph =
        paragraph_end == std::string_view::npos
            ? text.substr(paragraph_begin)
            : text.substr(paragraph_begin, paragraph_end - paragraph_begin);
    wrap_paragraph(lines, paragraph, available_width, style, metrics);
    if (paragraph_end == std::string_view::npos) {
      break;
    }
    paragraph_begin = paragraph_end + 1;
  }

  double width = 0.0;
  for (const WrappedLine &line : lines) {
    width = std::max(width, line.width);
  }
  const double height =
      static_cast<double>(lines.size()) * metrics.line_height(style);
  return {.lines = std::move(lines), .width = width, .height = height};
}

} // namespace detail
} // namespace sampan::layout
