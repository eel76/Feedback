#include "generator/io.h"

#include <fstream>
#include <sstream>

namespace generator::io {
  auto content(std::filesystem::path const& filename) -> std::string {
    if (!std::filesystem::exists(filename))
      throw std::invalid_argument{ "file not found" };

    std::ostringstream content;
    content << std::ifstream{ filename }.rdbuf();
    return content.str();
  }
} // namespace generator::io
