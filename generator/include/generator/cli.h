#pragma once
#include <filesystem>

namespace generator::cli {
  struct parameters {
    std::filesystem::path diff_filename;
    std::filesystem::path rules_filename;
    std::filesystem::path workflow_filename;
    std::filesystem::path sources_filename;
  };

  auto parse(int argc, char* argv[]) -> parameters;
} // namespace generator::cli
