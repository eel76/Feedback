#pragma once
#include <string>

namespace parameter {

  struct parameters {
    std::string output_filename;
    std::string diff_filename;
    std::string rules_filename;
    std::string workflow_filename;
    std::string sources_filename;
  };

  auto parse(int argc, char* argv[]) -> parameters;

} // namespace parameter
