#pragma once
#include <string>

namespace parameter {

struct parameters
{
  std::string rules_file_name;
  std::string sources_file_name;
  std::string files_to_check;
  std::string output_file_name;
  bool        treat_warnings_as_errors{ false };
};

auto parse(int argc, char* argv[]) -> parameters;

} // namespace parameter
