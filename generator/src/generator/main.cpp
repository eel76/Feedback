#include "generator/cli.h"
#include "generator/json.h"
#include "generator/output.h"
#include "generator/regex.h"
#include "generator/scm.h"
#include "generator/text.h"

#include <cxx20/syncstream>

#include <algorithm>
#include <execution>
#include <fstream>
#include <future>
#include <iostream>

namespace generator {

  template <class... Args> decltype(auto) launch_async(Args&&... args) {
    return std::async(std::launch::async, std::forward<Args>(args)...);
  }

  auto content(std::string const& filename) -> std::string {
    if (filename.empty())
      return {};

    auto input_stream = std::ifstream{ filename };

    std::stringstream content;
    content << input_stream.rdbuf();
    return content.str();
  }

  template <class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
  template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

  auto is_relevant_function(std::shared_future<feedback::workflow> const& shared_workflow,
                            std::shared_future<scm::diff> const&          shared_diff) {
    return [=](std::string_view filename) {
      auto const shared_file_changes = launch_async([=] { return shared_diff.get().changes_from(filename); }).share();

      return [=](feedback::rules::value_type const& rule) {
        auto const& [id, attributes] = rule;

        auto const rule_matched_filename = attributes.matched_files.matches(filename);
        auto const rule_ignored_filename = attributes.ignored_files.matches(filename);

        auto file_is_relevant = rule_matched_filename and not rule_ignored_filename;
        auto line_is_relevant = std::function<bool(int)>{ [](auto) { return true; } };

        if (file_is_relevant) {
          switch (container::value_or_default(shared_workflow.get(), attributes.type).check) {
          case feedback::check::NO_LINES:
            [[fallthrough]];
          case feedback::check::NO_FILES:
            file_is_relevant = false;
            break;
          case feedback::check::CHANGED_LINES:
            line_is_relevant = [is_modified = shared_file_changes.get()](auto line) { return is_modified[line]; };
            [[fallthrough]];
          case feedback::check::CHANGED_FILES:
            file_is_relevant = not shared_file_changes.get().empty();
            break;
          case feedback::check::ALL_LINES:
            [[fallthrough]];
          case feedback::check::ALL_FILES:
            break;
          }
        }

        if (!file_is_relevant) {
          line_is_relevant = [](auto) { return false; };
        }

        return overloaded{ [=]() { return file_is_relevant; }, [=](auto line) { return line_is_relevant(line); } };
      };
    };
  }

  struct rule_in_source_matches {
    feedback::rules::value_type const&     rule;
    std::shared_future<std::string> const& shared_content;
  };

  struct source_matches {
    std::shared_future<feedback::rules> const& shared_rules;
    std::shared_future<std::string> const&     shared_content;
  };

  struct all_matches {
    std::shared_future<feedback::rules> const&          shared_rules;
    std::shared_future<feedback::workflow> const&       shared_workflow;
    std::shared_future<std::vector<std::string>> const& shared_sources;
    std::shared_future<scm::diff> const&                shared_diff;
  };

  template <class FUNCTION> auto print(std::ostream& out, rule_in_source_matches matches, FUNCTION is_relevant) {
    auto const& [id, attributes] = matches.rule;

    auto search = text::forward_search{ matches.shared_content.get() };

    while (search.next_but(attributes.matched_text, attributes.ignored_text)) {
      auto const line_number = search.line() + 1;
      if (not is_relevant(line_number))
        continue;

      print(out, output::match{ id, line_number, search.highlighted_text(attributes.marked_text) });
    }
  }

  template <class FUNCTION> void print(std::ostream& out, source_matches matches, FUNCTION is_relevant_in_source) {
    auto const& rules = matches.shared_rules.get();

    std::for_each(std::execution::par, cbegin(rules), cend(rules), [=, &out](auto const& rule) {
      auto const is_rule_in_source_relevant = is_relevant_in_source(rule);
      if (not is_rule_in_source_relevant())
        return;

      auto synchronized_out = cxx20::osyncstream{ out };

      print(synchronized_out, rule_in_source_matches{ rule, matches.shared_content }, is_rule_in_source_relevant);
    });
  }

  void print(std::ostream& out, all_matches matches) {
    auto const  is_relevant = is_relevant_function(matches.shared_workflow, matches.shared_diff);
    auto const& sources     = matches.shared_sources.get();

    std::for_each(std::execution::par, cbegin(sources), cend(sources), [=, &out](std::string const& filename) {
      auto const shared_content = launch_async([=] { return content(filename); }).share();

      auto synchronized_out = cxx20::osyncstream{ out };

      print(synchronized_out, output::source{ filename });
      print(synchronized_out, source_matches{ matches.shared_rules, shared_content }, is_relevant(filename));
    });
  }

  auto parse_diff_async(std::string const& filename) {
    return launch_async([=] {
      scm::diff accumulated;
      if (not filename.empty())
        accumulated = scm::diff::parse(content(filename), std::move(accumulated));
      return accumulated;
    });
  }

  auto parse_rules_async(std::string const& filename) {
    return launch_async([=] { return json::parse_rules(content(filename)); });
  }

  auto parse_workflow_async(std::string const& filename) {
    return launch_async([=] { return json::parse_workflow(content(filename)); });
  }

  auto parse_sources_async(std::string const& filename) {
    return launch_async([=] {
      auto sources = std::vector<std::string>{};
      auto content = std::ifstream{ filename };

      for (std::string source; std::getline(content, source);)
        sources.emplace_back(source);

      return sources;
    });
  }
} // namespace generator

int main(int argc, char* argv[]) {
  std::ios::sync_with_stdio(false);

  std::stringstream out;

  try {
    auto const parameters = generator::cli::parse(argc, argv);

    auto const shared_diff     = generator::parse_diff_async(parameters.diff_filename).share();
    auto const shared_rules    = generator::parse_rules_async(parameters.rules_filename).share();
    auto const shared_workflow = generator::parse_workflow_async(parameters.workflow_filename).share();
    auto const shared_sources  = generator::parse_sources_async(parameters.sources_filename).share();

    print(out, generator::output::header{ shared_rules, shared_workflow, parameters.rules_filename });
    print(out, generator::all_matches{ shared_rules, shared_workflow, shared_sources, shared_diff });
  }
  catch (std::exception const& e) {
    std::cerr << e.what() << '\n';
    return 1;
  }

  std::cout << out.rdbuf();
  return 0;
}
