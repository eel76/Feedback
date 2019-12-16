#pragma once

#include <memory>
#include <string_view>

namespace regex {

using match_result = std::string_view*;
using match_results = std::initializer_list<match_result>;

struct precompiled
{
  precompiled() = default;

  auto matches(std::string_view input) const -> bool;
  auto matches(std::string_view input, match_results captures_ret) const -> bool;

  auto find_first(std::string_view input, match_result match_ret, match_result skipped_ret, match_result remaining_ret) const
    -> bool;

private:
  explicit precompiled(std::string_view pattern);
  friend auto compile(std::string_view pattern) -> precompiled;

private:
  struct impl;
  std::shared_ptr<impl> engine;
};

auto compile(std::string_view pattern) -> precompiled;
auto capture(std::string_view pattern) -> std::string;

} // namespace regex
