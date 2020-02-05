#pragma once
#include <string>

namespace parameter {

struct parameters
{
  std::string coding_guidelines;
  std::string source_files;
  std::string files_to_check;
  std::string output_file;
};

auto parse(int argc, char* argv[]) -> parameters;

} // namespace parameter
