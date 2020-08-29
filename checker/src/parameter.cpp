#include "feedback/parameter.h"

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wtype-limits"
#endif

#include <lyra/lyra.hpp>

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

auto parameter::parse(int argc, char* argv[]) -> parameters
{
  parameters p;

  auto const cli =
    lyra::arg(p.rules_file, "codingGuidelines")("Coding guidelines (JSON file) to check") |
    lyra::arg(p.source_list_file, "source files")("File with list of source files to scan") |
    lyra::opt(p.files_to_check, "files to check")["-f"]["--files-to-check"]("File with list of files to check") |
    lyra::opt(p.output_file, "output file")["-o"]["--output-file"]("Output file name");

  auto const result = cli.parse({ argc, argv });
  if (!result)
    throw std::invalid_argument{ "Error in command line: " + result.errorMessage() };

  return p;
}

