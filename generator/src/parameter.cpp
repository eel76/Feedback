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
    lyra::opt(p.treat_warnings_as_errors, "treat warnings as errors")["-e"]["--errors"]("Report all warnings as errors") |
    lyra::opt(p.files_to_check, "files to check")["-f"]["--files-to-check"]("File with list of files to check") |
    lyra::opt(p.output_file_name, "output file")["-o"]["--output-file"]("Output file name") |
    lyra::arg(p.rules_file_name, "rules")("Feedback rules") |
    lyra::arg(p.sources_file_name, "source files")("File with list of source files to scan");

  auto const result = cli.parse({ argc, argv });
  if (!result)
    throw std::invalid_argument{ result.errorMessage() };

  return p;
}

