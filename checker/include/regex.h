#pragma once

#include <memory>
#include <string_view>

namespace re2 {
class RE2;
}

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
  std::shared_ptr<re2::RE2> engine;
};

auto sub_pattern(std::string_view pattern) -> precompiled;

} // namespace regex
