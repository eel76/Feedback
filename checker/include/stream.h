#pragma once

#include <fstream>
#include <iostream>
#include <string>

namespace stream {

auto content(std::istream& in)
{
  std::stringstream sstr;
  sstr << in.rdbuf();
  return sstr.str();
}

auto read_from(std::string const& file_name)
{
  struct temporary_stream
  {
    explicit temporary_stream(std::string const& file_name)
    : stream(file_name)
    {
    }
    operator std::istream&() &&
    {
      return stream;
    }

  private:
    std::ifstream stream;
  };

  return temporary_stream{ file_name };
}

template <class Operation>
void for_each_line(std::istream& is, Operation&& operation)
{
  for (std::string line; std::getline(is, line);)
    operation(line);
}

template <class Operation>
void for_each_line_read_from(std::string const& file_name, Operation&& operation)
{
  for_each_line(read_from(file_name), std::forward<Operation>(operation));
}

} // namespace stream
