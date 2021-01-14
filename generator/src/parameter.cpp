#include "feedback/parameter.h"

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wtype-limits"
#endif

#include <lyra/lyra.hpp>

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

auto parameter::parse(int argc, char* argv[]) -> parameters {
  parameters p;

  auto const cli = lyra::opt(p.output_filename, "output filename")["-o"]["--output"]("Output file name") |
                   lyra::opt(p.diff_filename, "diff filename")["-d"]["--diff"]("diff file name") |
                   lyra::arg(p.rules_filename, "rules filename")("JSON file for feedback rules") |
                   lyra::arg(p.workflow_filename, "workflow filename")("JSON file for workflow") |
                   lyra::arg(p.sources_filename, "sources filename")("File list for source files to scan");

  if (auto const result = cli.parse({ argc, argv }); not result)
    throw std::invalid_argument{ result.errorMessage() };

  return p;
}
