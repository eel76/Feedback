#pragma once

#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>

namespace io {

auto content(std::istream& in)
{
  std::stringstream sstr;
  sstr << in.rdbuf();
  return sstr.str();
}

auto stream_from(std::string const& file_name)
{
  struct temporary_stream
  {
    explicit temporary_stream(std::string const& file_name)
    : stream(file_name)
    {
    }
    operator std::istream &() &&
    {
      return stream;
    }

  private:
    std::ifstream stream;
  };

  return temporary_stream{ file_name };
}

auto redirect(std::ostream& stream, std::string const& file_name)
{
  struct redirection
  {
    redirection(std::ostream& stream, std::string const& file_name)
    : output_stream(stream)
    , file_stream(file_name)
    , backup(output_stream.rdbuf(file_stream.rdbuf()))
    {
    }
    ~redirection()
    {
      output_stream.rdbuf(backup);
    }

  private:
    std::ostream& output_stream;
    std::ofstream file_stream;
    std::streambuf* backup;
  };

  if (file_name.empty())
    return std::optional<redirection>{};

  return std::make_optional<redirection>(stream, file_name);
}

template <class Operation>
void for_each_line(std::istream& is, Operation&& operation)
{
  for (std::string line; std::getline(is, line);)
    operation(line);
}

} // namespace stream
