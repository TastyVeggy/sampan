#include <gtest/gtest.h>

#include <sstream>

#include "sampan/core/log.hpp"

namespace {

TEST(Log, FiltersBelowConfiguredLevel) {
  std::ostringstream output;
  sampan::core::Log log{output};
  log.set_level(sampan::core::LogLevel::Warn);

  log.write(sampan::core::LogLevel::Info, "core", "hidden");
  log.write(sampan::core::LogLevel::Error, "core", "shown");

  EXPECT_EQ(output.str(), "ERROR core shown\n");
}

TEST(Log, ProvidesStableLevelNames) {
  EXPECT_EQ(sampan::core::to_string(sampan::core::LogLevel::Trace), "TRACE");
  EXPECT_EQ(sampan::core::to_string(sampan::core::LogLevel::Debug), "DEBUG");
  EXPECT_EQ(sampan::core::to_string(sampan::core::LogLevel::Info), "INFO");
  EXPECT_EQ(sampan::core::to_string(sampan::core::LogLevel::Warn), "WARN");
  EXPECT_EQ(sampan::core::to_string(sampan::core::LogLevel::Error), "ERROR");
}

} // namespace
