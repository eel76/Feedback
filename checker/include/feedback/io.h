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

} // namespace stream
