#pragma once
#include <filesystem>
#include <string>

namespace generator::io {
  auto content(std::filesystem::path const& filename) -> std::string;
} // namespace generator::io
