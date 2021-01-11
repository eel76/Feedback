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

  auto redirect(std::ostream& stream, std::string const& filename)
  {
    struct redirection
    {
      redirection(std::ostream& stream, std::string const& filename)
        : output_stream(stream)
        , file_stream(filename)
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

    if (filename.empty())
      return std::optional<redirection>{};

    return std::make_optional<redirection>(stream, filename);
  }

} // namespace stream
