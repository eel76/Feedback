#include "generator/cli.h"

#include "feedback/macros.h"

COMPILER_WARNINGS_PUSH

MSC_WARNING(disable : 4100)
MSC_WARNING(disable : 4458)
GCC_DIAGNOSTIC(ignored "-Wtype-limits")

#include <lyra/lyra.hpp>

COMPILER_WARNINGS_POP

auto generator::cli::parse(int argc, char* argv[]) -> generator::cli::parameters {
  generator::cli::parameters p;

  auto const cli = lyra::opt(p.diff_filename, "diff filename")["-d"]["--diff"]("diff file name") |
                   lyra::arg(p.rules_filename, "rules filename")("JSON file for feedback rules") |
                   lyra::arg(p.workflow_filename, "workflow filename")("JSON file for workflow") |
                   lyra::arg(p.sources_filename, "sources filename")("File list for source files to scan");

  if (auto const result = cli.parse({ argc, argv }); not result)
    throw std::invalid_argument{ result.errorMessage() };

  return p;
}
