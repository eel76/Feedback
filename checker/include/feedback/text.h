#pragma once

#include <string_view>

namespace text {

auto first_line_of(std::string_view text)
{
  auto const end_of_first_line = text.find_first_of('\n');
  if (end_of_first_line == std::string_view::npos)
    return text;

  return text.substr(0, end_of_first_line);
}

auto last_line_of(std::string_view text)
{
  auto const end_of_penultimate_line = text.find_last_of('\n');
  if (end_of_penultimate_line == std::string_view::npos)
    return text;

  return text.substr(end_of_penultimate_line + 1);
}

} // namespace text
