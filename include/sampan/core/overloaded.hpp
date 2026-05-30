#pragma once

namespace sampan::core {

template <typename... Functions> struct Overloaded : Functions... {
  using Functions::operator()...;
};

} // namespace sampan::core
