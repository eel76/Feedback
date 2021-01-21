#pragma once
#include <string>

namespace generator::cli {
  struct parameters {
    std::string diff_filename;
    std::string rules_filename;
    std::string workflow_filename;
    std::string sources_filename;
  };

  auto parse(int argc, char* argv[]) -> parameters;
} // namespace generator::cli
