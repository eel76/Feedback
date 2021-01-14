#pragma once
#include <future>

namespace feedback::async {

  template <class... Args> decltype(auto) launch(Args&&... args) {
    return std::async(std::launch::async, std::forward<Args>(args)...);
  }

  template <class... Args> decltype(auto) share(Args&&... args) {
    return async::launch(std::forward<Args>(args)...).share();
  }
} // namespace feedback::async
