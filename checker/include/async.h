#pragma once

#include <future>
#include <vector>

namespace async {

template <class... Args>
decltype(auto) launch(Args&&... args)
{
  return std::async(std::launch::async, std::forward<Args>(args)...);
}

template <class... Args>
auto share(Args&&... args)
{
  return async::launch(std::forward<Args>(args)...).share();
}

template <class T>
struct waitable_range
{
  ~waitable_range()
  {
    for (auto&& f : *static_cast<T*>(this))
      f.wait();
  }
};

template <class T>
using future_vector = std::vector<std::future<T>>;

template <class Op>
auto make_task(Op&& operation)
{
  struct async_tasks : future_vector<void>, waitable_range<async_tasks>
  {
  };

  return [tasks = async_tasks{}, operation = std::forward<Op>(operation)](auto&&... args) mutable {
    tasks.emplace_back(async::launch([operation, args = std::make_tuple(std::forward<decltype(args)>(args)...)] {
      std::apply(operation, std::move(args));
    }));
  };
}

} // namespace async
