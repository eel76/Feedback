#pragma once

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace stream {

auto content(std::istream& in)
{
  std::stringstream sstr;
  sstr << in.rdbuf();
  return sstr.str();
}

auto from(std::string const& file_name)
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

struct line : public std::string
{
  friend std::istream& operator>>(std::istream& is, line& val)
  {
    return std::getline(is, val);
  }
};

struct all_lines
{
  explicit all_lines(std::istream& is)
  : stream(is)
  {
  }

  friend auto begin(all_lines const& lines)
  {
    return std::istream_iterator<line>{ lines.stream };
  }

  friend auto end(all_lines const&)
  {
    return std::istream_iterator<line>{};
  }

private:
  std::istream& stream;
};

template <class Operation>
void for_each_line(std::istream& is, Operation&& operation)
{
  for (auto&& line : all_lines {is})
    operation(line);
}

//template <class Operation>
//void for_each_line(std::istream& is, Operation&& operation)
//{
//  for (std::string line; std::getline(is, line);)
//    operation(line);
//}

} // namespace stream
