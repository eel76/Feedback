#pragma once

#include <future>
#include <vector>

namespace async {

template <class... Args>
decltype(auto) launch(Args&&... args)
{
  return std::async(/* FIXME */ std::launch::deferred, std::forward<Args>(args)...);
}

template <class... Args>
auto share(Args&&... args)
{
  return async::launch(std::forward<Args>(args)...).share();
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

template <class Op>
auto as_task(Op&& operation)
{
  return [tasks = std::vector<task>{}, operation = std::forward<Op>(operation)](auto&&... args) mutable {
    tasks.emplace_back([operation, args = std::make_tuple(std::forward<decltype(args)>(args)...)] {
      std::apply(operation, std::move(args));
    });
  };
}

} // namespace async
