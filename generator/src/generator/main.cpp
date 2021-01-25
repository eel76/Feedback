#include "generator/async.h"
#include "generator/cli.h"
#include "generator/format.h"
#include "generator/json.h"
#include "generator/output.h"
#include "generator/regex.h"
#include "generator/scm.h"
#include "generator/text.h"

#include <cxx20/syncstream>

#include <algorithm>
#include <execution>
#include <fstream>
#include <iostream>

namespace generator {

  auto content(std::string const& filename) -> std::string {
    if (filename.empty())
      return {};

    auto input_stream = std::ifstream{ filename };

    std::stringstream content;
    content << input_stream.rdbuf();
    return content.str();
  }

  auto search_marked_text(std::string_view const& text, regex::precompiled const& pattern) {
    auto search = text::forward_search{ text };

    if (search.next(pattern))
      return search.matched_text();

    return text;
  }

  template <class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
  template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

  auto is_relevant_function(std::shared_future<feedback::workflow> const& shared_workflow,
                            std::shared_future<scm::diff> const&          shared_diff) {
    return [=](std::string_view filename) {
      auto const shared_file_changes = async::share([=] { return shared_diff.get().changes_from(filename); });

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

  template <class FUNCTION> auto print(std::ostream& out, rule_in_source_matches matches, FUNCTION is_relevant) {
    auto const& [id, attributes] = matches.rule;

    auto search = text::forward_search{ matches.shared_content.get() };

    while (search.next(attributes.matched_text)) {
      auto const matched_lines = search.matched_lines();
      if (attributes.ignored_text.matches(matched_lines))
        continue;

      auto const line_number = search.line() + 1;
      if (not is_relevant(line_number))
        continue;

      auto const marked_text = search_marked_text(search.matched_text(), attributes.marked_text);
      print(out, output::match{ id, line_number, output::excerpt{ matched_lines, marked_text } });
    }
  }

  struct source_matches {
    std::shared_future<feedback::rules> const& shared_rules;
    std::shared_future<std::string> const&     shared_content;
  };

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

  struct all_matches {
    std::shared_future<feedback::rules> const&          shared_rules;
    std::shared_future<feedback::workflow> const&       shared_workflow;
    std::shared_future<std::vector<std::string>> const& shared_sources;
    std::shared_future<scm::diff> const&                shared_diff;
  };

  void print(std::ostream& out, all_matches matches) {
    auto const  is_relevant = is_relevant_function(matches.shared_workflow, matches.shared_diff);
    auto const& sources     = matches.shared_sources.get();

    std::for_each(std::execution::par, cbegin(sources), cend(sources), [=, &out](std::string const& filename) {
      auto const shared_content = async::share([=] { return content(filename); });

      auto synchronized_out = cxx20::osyncstream{ out };

      print(synchronized_out, output::source{ filename });
      print(synchronized_out, source_matches{ matches.shared_rules, shared_content }, is_relevant(filename));
    });
  }

  auto parse_sources(std::string const& filename) {
    auto sources = std::vector<std::string>{};
    auto content = std::ifstream{ filename };

    for (std::string source; std::getline(content, source);)
      sources.emplace_back(source);

    return sources;
  }

  auto parse_diff_async(std::string const& filename) {
    return async::share([=] {
      scm::diff accumulated;
      if (not filename.empty())
        accumulated = scm::diff::parse(content(filename), std::move(accumulated));
      return accumulated;
    });
  }

  auto parse_rules_async(std::string const& filename) {
    return async::share([=] { return json::parse_rules(content(filename)); });
  }

  auto parse_workflow_async(std::string const& filename) {
    return async::share([=] { return json::parse_workflow(content(filename)); });
  }

  auto parse_sources_async(std::string const& filename) {
    return async::share([=] { return parse_sources(filename); });
  }
} // namespace generator

using namespace generator;

int main(int argc, char* argv[]) {
  std::ios::sync_with_stdio(false);

  std::stringstream out;

  try {
    auto const parameters = cli::parse(argc, argv);

    auto const shared_diff     = parse_diff_async(parameters.diff_filename);
    auto const shared_rules    = parse_rules_async(parameters.rules_filename);
    auto const shared_workflow = parse_workflow_async(parameters.workflow_filename);
    auto const shared_sources  = parse_sources_async(parameters.sources_filename);

    print(out, output::header{ shared_rules, shared_workflow, parameters.rules_filename });
    print(out, all_matches{ shared_rules, shared_workflow, shared_sources, shared_diff });
  }
  catch (std::exception const& e) {
    std::cerr << e.what() << '\n';
    return 1;
  }

  std::cout << out.rdbuf();
  return 0;
}
