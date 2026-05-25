#include <gtest/gtest.h>

#include "sampan/core/diagnostic.hpp"

namespace {

TEST(Diagnostic, SeverityNamesAreStable) {
  EXPECT_EQ(sampan::core::to_string(sampan::core::Severity::Note), "note");
  EXPECT_EQ(sampan::core::to_string(sampan::core::Severity::Warning),
            "warning");
  EXPECT_EQ(sampan::core::to_string(sampan::core::Severity::Error), "error");
}

TEST(SourceSpan, ValidatesHalfOpenOffsets) {
  EXPECT_TRUE((sampan::core::SourceSpan{
                   .file_id = 1, .start_offset = 4, .end_offset = 4})
                  .valid());
  EXPECT_FALSE((sampan::core::SourceSpan{
                    .file_id = 1, .start_offset = 5, .end_offset = 4})
                   .valid());
}

TEST(SourceSpan, DefaultsToTheFirstSourceCoordinate) {
  const sampan::core::SourceSpan span;
  EXPECT_EQ(span.start_line, 1);
  EXPECT_EQ(span.start_column, 1);
  EXPECT_EQ(span.end_line, 1);
  EXPECT_EQ(span.end_column, 1);
}

} // namespace
