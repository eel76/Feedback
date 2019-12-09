#pragma once

#include <memory>
#include <string_view>

namespace regex {

struct precompiled
{
  // FIXME: use a match type which inherits from string_view, has prefix and suffix members, too

  precompiled() = default;
  explicit precompiled(std::string_view pattern);

  friend bool search(std::string_view input, precompiled const& pattern);
  friend bool search(std::string_view input, precompiled const& pattern, std::string_view* match);

  friend bool
  search(std::string_view input, precompiled const& pattern, std::string_view* match1, std::string_view* match2);

  friend bool search_next(std::string_view* input, precompiled const& pattern, std::string_view* match);

private:
  struct impl;
  std::shared_ptr<impl> engine;
};

auto sub_pattern(std::string_view pattern) -> precompiled;

} // namespace regex
