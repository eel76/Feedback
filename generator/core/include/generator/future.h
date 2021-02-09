#pragma once
#include <future>

namespace generator::future {
  template <class... Args> decltype(auto) launch_async(Args&&... args) {
    return std::async(std::launch::async, std::forward<Args>(args)...);
  }
} // namespace generator::future
