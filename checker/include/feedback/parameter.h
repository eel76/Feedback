#pragma once
#include <string>

namespace parameter {

struct parameters
{
  std::string rules_file;
  std::string source_list_file;
  std::string files_to_check;
  std::string output_file;
};

auto parse(int argc, char* argv[]) -> parameters;

} // namespace parameter
