#include "generator/cli.h"
#include "generator/io.h"
#include "generator/json.h"
#include "generator/output.h"
#include "generator/scm.h"

#include <fstream>
#include <future>
#include <iostream>
#include <sstream>

namespace generator {

  auto parse_rules_async(std::filesystem::path const& filename) {
    return std::async(std::launch::async, [=] { return json::parse_rules(io::content(filename)); });
  }

  auto parse_sources_async(std::filesystem::path const& filename) {
    return std::async(std::launch::async, [=] {
      if (!std::filesystem::exists(filename))
        throw std::invalid_argument{ "file not found" };

      // FIXME: use mmap

      std::stringstream content;
      content << std::ifstream{ filename }.rdbuf();

      auto sources = std::vector<std::filesystem::path>{};

      for (std::string source; std::getline(content, source);)
        sources.emplace_back(source);

      return sources;
    });
  }

  auto parse_workflow_async(std::filesystem::path const& filename) {
    return std::async(std::launch::async, [=] {
      if (not filename.empty())
        return json::parse_workflow(io::content(filename));
      return feedback::workflow{};
    });
  }

  auto parse_diff_async(std::filesystem::path const& filename) {
    return std::async(std::launch::async, [=] {
      scm::diff accumulated;
      if (not filename.empty())
        accumulated = scm::diff::parse(io::content(filename), std::move(accumulated));
      return accumulated;
    });
  }
} // namespace generator

int main(int argc, char* argv[]) {
  using namespace generator;

  std::ostringstream out;

  try {
    auto const parameters = cli::parse(argc, argv);

    auto const shared_rules    = parse_rules_async(parameters.rules_filename).share();
    auto const shared_sources  = parse_sources_async(parameters.sources_filename).share();
    auto const shared_workflow = parse_workflow_async(parameters.workflow_filename).share();
    auto const shared_diff     = parse_diff_async(parameters.diff_filename).share();

    print(out, output::matches{ parameters.rules_filename, shared_rules, shared_sources, shared_workflow, shared_diff });
  }
  catch (std::exception const& e) {
    std::cerr << e.what() << '\n';
    return 1;
  }

  std::ios::sync_with_stdio(false);
  std::cout << out.str();
  return 0;
}
