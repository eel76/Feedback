#pragma once

#include <future>
#include <vector>

namespace async {

template <class... Args>
decltype(auto) launch(Args&&... args)
{
  return std::async(std::launch::async, std::forward<Args>(args)...);
}

struct task
{
  template <class Callable>
  explicit task(Callable&& callable)
  : call(async::launch(std::forward<Callable> (callable)))
  {
  }

  task(task&&) = default;
  task& operator=(task&&) = default;

  ~task()
  {
    if (call.valid())
      call.wait();
  }

 private:
  std::future<void> call;
};

} // namespace async
