#pragma once

#include "format.h"

#include <chrono>
#include <iostream>
#include <string>
#include <string_view>
#include <syncstream>

namespace benchmark {

template <class T>
struct duration_scope
{
  duration_scope(std::string_view format)
  : format(format)
  , started(std::chrono::system_clock::now())
  {
  }

  duration_scope(duration_scope&&) = delete;
  duration_scope(duration_scope const&) = delete;

  ~duration_scope()
  {
    auto const elapsed = std::chrono::system_clock::now() - started;
    format::print(std::osyncstream{ std::cerr }, format, std::chrono::duration_cast<T>(elapsed).count());
  }

private:
  std::string format;
  std::chrono::system_clock::time_point started;
};

} // namespace benchmark
