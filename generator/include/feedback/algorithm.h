#pragma once
#include <algorithm>
#include <execution>

#include "feedback/async.h"

namespace feedback::algorithm {

  template <class RANGE, class FUNCTOR> void for_each(std::execution::parallel_policy, RANGE&& range, FUNCTOR&& functor) {

    //std::for_each(std::execution::seq, cbegin(range), cend(range), std::forward<FUNCTOR> (functor));

    auto tasks = std::vector<std::future<void>>{};
    tasks.reserve(size(range));

    for (auto&& element : range)
      tasks.push_back(async::launch(functor, element));

    for (auto&& task : tasks)
      if (task.valid())
        task.wait();
  }
} // namespace feedback::algorithm
