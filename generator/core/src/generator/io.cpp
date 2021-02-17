#include "generator/io.h"

#include <mio/mmap.hpp>

#include <fstream>
#include <sstream>

namespace generator::io {
  auto content(std::filesystem::path const& filename) -> std::string {
    if (!std::filesystem::exists(filename))
      throw std::invalid_argument{ "file not found" };

    //if (auto const mmap = mio::mmap_source{ filename.native() }; mmap.is_open())
    //  return { mmap.cbegin(), mmap.cend() };

    std::ostringstream content;
    content << std::ifstream{ filename }.rdbuf();
    return content.str();
  }
} // namespace generator::io
