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
    lyra::opt(p.files_to_check, "files to check")["-f"]["--files-to-check"]("File with list of files to check") |
    lyra::opt(p.output_filename, "output filename")["-o"]["--output-file"]("Output file name") |
    lyra::opt(p.workflow_filename, "workflow filename")["-w"]["--workflow"]("JSON file for workflow") |
    lyra::opt(p.diff_filename, "diff filename")["-d"]["--diff"]("diff output for changed code lines") |
    lyra::arg(p.rules_filename, "rules filename")("JSON file for feedback rules") |
    lyra::arg(p.sources_filename, "sources filename")("File for list of source files to scan");

  auto const result = cli.parse({ argc, argv });
  if (!result)
    throw std::invalid_argument{ result.errorMessage() };

  return p;
}

