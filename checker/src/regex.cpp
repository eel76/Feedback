#include "regex.h"

#include <re2/re2.h>

#include <cassert>
#include <string_view>
#include <vector>

namespace {
auto as_string_piece(std::string_view sv)
{
  return re2::StringPiece{ sv.data(), sv.length() };
}

auto as_string_view(re2::StringPiece const& match)
{
  return std::string_view{ match.data(), match.length() };
}
} // namespace

struct regex::precompiled::impl : public re2::RE2
{
  explicit impl(std::string_view pattern)
  : RE2(as_string_piece(pattern), RE2::Quiet)
  {
  }
};

regex::precompiled::precompiled(std::string_view pattern)
: engine(std::make_shared<impl>(pattern))
{
  if (not engine->ok())
    throw std::invalid_argument{ std::string{ "Invalid regex: " }.append(engine->error()) };
}

bool regex::search(std::string_view input, precompiled const& pattern)
{
  return RE2::PartialMatch(as_string_piece(input), *pattern.engine);
}

bool regex::search(std::string_view input, precompiled const& pattern, std::string_view* match)
{
  auto re2_match = as_string_piece(*match);

  assert(pattern.engine->NumberOfCapturingGroups() >= 1);

  if (not RE2::PartialMatch(as_string_piece(input), *pattern.engine, &re2_match))
    return false;

  *match = as_string_view(re2_match);

  return true;
}

bool regex::search(
  std::string_view input, precompiled const& pattern, std::string_view* match1, std::string_view* match2)
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

bool regex::search_next(std::string_view* input, precompiled const& pattern, std::string_view* match)
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

auto regex::compile(std::string_view pattern) -> precompiled
{
  return precompiled{ pattern };
}

auto regex::capture(std::string_view pattern) -> std::string
{
  return std::string{ "(" }.append(pattern).append(")");
}
