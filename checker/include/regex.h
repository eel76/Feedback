#pragma once

#include <memory>
#include <string_view>

namespace regex {

struct precompiled
{
  // FIXME: use a match type which inherits from string_view, has prefix and suffix members, too

  precompiled() = default;

  friend auto search(std::string_view input, precompiled const& pattern) -> bool;
  friend auto search(std::string_view input, precompiled const& pattern, std::string_view* match) -> bool;

  friend auto
  search(std::string_view input, precompiled const& pattern, std::string_view* match1, std::string_view* match2)
    -> bool;

  friend auto search_next(std::string_view* input, precompiled const& pattern, std::string_view* match) -> bool;

  friend auto compile(std::string_view pattern) -> precompiled;

private:
  explicit precompiled(std::string_view pattern);

private:
  struct impl;
  std::shared_ptr<impl> engine;
};

auto compile(std::string_view pattern) -> precompiled;
auto capture(std::string_view pattern) -> std::string;

} // namespace regex
