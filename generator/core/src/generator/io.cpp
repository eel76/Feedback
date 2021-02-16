#include "generator/io.h"

#include <mio/mmap.hpp>

namespace generator::io {
  auto content(std::filesystem::path const& filename) -> std::string {
    if (!std::filesystem::exists(filename))
      throw std::invalid_argument{ "file not found" };

    auto const mmap = mio::mmap_source{ filename.native() };
    return { mmap.cbegin(), mmap.cend() };
  }
} // namespace generator::io
