#pragma once

#include <optional>
#include <string>

namespace sampan::app {

[[nodiscard]] int run(int argc, char *argv[],
                      std::optional<std::string> initial_location);

} // namespace sampan::app
