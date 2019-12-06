#pragma once

#include <re2/re2.h>

#include <cassert>
#include <memory>
#include <string_view>

namespace regex {

struct precompiled
{
  precompiled() = default;

  explicit precompiled(std::string_view pattern)
  : engine(std::make_shared<re2::RE2>(as_string_piece(pattern), RE2::Quiet))
  {
    if (not engine->ok())
      throw std::invalid_argument{ std::string{ "Invalid regex: " }.append(engine->error()) };
  }

  friend bool search(std::string_view input, precompiled const& pattern)
  {
    return RE2::PartialMatch(as_string_piece(input), *pattern.engine);
  }

  friend bool search(std::string_view input, precompiled const& pattern, std::string_view* match)
  {
    auto re2_match = as_string_piece(*match);

    assert(pattern.engine->NumberOfCapturingGroups() >= 1);

    if (not RE2::PartialMatch(as_string_piece(input), *pattern.engine, &re2_match))
      return false;

    *match = as_string_view(re2_match);

    return true;
  }

  friend bool
  search(std::string_view input, precompiled const& pattern, std::string_view* match1, std::string_view* match2)
  {
    auto re2_match1 = as_string_piece(*match1);
    auto re2_match2 = as_string_piece(*match2);

    assert(pattern.engine->NumberOfCapturingGroups() >= 2);

    if (not RE2::PartialMatch(as_string_piece(input), *pattern.engine, &re2_match1, &re2_match2))
      return false;

    *match1 = as_string_view(re2_match1);
    *match2 = as_string_view(re2_match2);

    return true;
  }

  friend bool search_next(std::string_view* input, precompiled const& pattern, std::string_view* match)
  {
    auto re2_input = as_string_piece(*input);
    auto re2_match = as_string_piece(*match);

    assert(pattern.engine->NumberOfCapturingGroups() >= 1);

    if (not RE2::FindAndConsume(&re2_input, *pattern.engine, &re2_match))
      return false;

    *input = as_string_view(re2_input);
    *match = as_string_view(re2_match);

    return true;
  }

private:
  static auto as_string_piece(std::string_view sv) -> re2::StringPiece
  {
    return re2::StringPiece{ sv.data(), sv.length() };
  }

  static auto as_string_view(re2::StringPiece const& match) -> std::string_view
  {
    return std::string_view{ match.data(), match.length() };
  }

private:
  std::shared_ptr<re2::RE2> engine;
};

auto sub_pattern(std::string_view pattern)
{
  return precompiled{ std::string{ "(" }.append(pattern).append(")") };
}

} // namespace regex
