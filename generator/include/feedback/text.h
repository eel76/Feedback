#pragma once

#include <string_view>

namespace regex {
class precompiled;
}

namespace text {

inline auto first_line_of(std::string_view text) -> std::string_view
{
  auto const end_of_first_line = text.find_first_of('\n');
  if (end_of_first_line == std::string_view::npos)
    return text;

  return text.substr(0, end_of_first_line);
}

inline auto last_line_of(std::string_view text) -> std::string_view
{
  auto const end_of_penultimate_line = text.find_last_of('\n');
  if (end_of_penultimate_line == std::string_view::npos)
    return text;

  return text.substr(end_of_penultimate_line + 1);
}

class forward_search
{
public:
  explicit forward_search(std::string_view const& text)
  : processed_line_count(0)
  , processed(text.data(), 0)
  , current_match(text.data(), 0)
  , remaining(text)
  {
  }

  auto next(regex::precompiled const& pattern) -> bool;

  auto matched_text() const->std::string_view
  {
    return current_match;
  }

  auto matched_lines() const -> std::string_view;

  auto line() const -> ptrdiff_t
  {
    return processed_line_count;
  }

  auto column() const -> ptrdiff_t
  {
    return last_processed_line.length();
  }

private:
  void skip(std::string_view const& text);

private:
  ptrdiff_t processed_line_count;
  std::string_view processed;
  std::string_view last_processed_line;
  std::string_view current_match;
  std::string_view first_remaining_line;
  std::string_view remaining;
};

} // namespace text