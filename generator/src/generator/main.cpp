#include "feedback/async.h"
#include "feedback/format.h"
#include "feedback/json.h"
#include "feedback/output.h"
#include "feedback/regex.h"
#include "feedback/scm.h"
#include "feedback/text.h"

#include "generator/cli.h"

#include <cxx20/syncstream>

#include <algorithm>
#include <execution>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace feedback {

  namespace {

    auto content(std::string const& filename) -> std::string {
      if (filename.empty())
        return {};

      auto input_stream = std::ifstream{ filename };

      std::stringstream content;
      content << input_stream.rdbuf();
      return content.str();
    }
  } // namespace

  auto search_marked_text(std::string_view const& text, regex::precompiled const& pattern) {
    auto search = text::forward_search{ text };

    if (search.next(pattern))
      return search.matched_text();

    return text;
  }

  class excerpt {
  public:
    excerpt(std::string_view text, std::string_view match) {
      assert(text.data() <= match.data());
      assert(text.data() + text.length() >= match.data() + match.length());

      first_line = text::first_line_of(text);

      indentation.append(match.data() - text.data(), ' ');
      annotation.append(text::first_line_of(match).length(), '~');

      if (not annotation.empty())
        annotation[0] = '^';
    }

  public:
    std::string_view first_line;
    std::string      indentation;
    std::string      annotation;
  };

  namespace {
    template <class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
    template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;
  } // namespace

  auto is_relevant_function(std::shared_future<control::workflow> const& shared_workflow,
                            std::shared_future<scm::diff> const&         shared_diff) {
    return [=](std::string_view filename) {
      auto const shared_file_changes = async::share([=] { return shared_diff.get().changes_from(filename); });

      return [=](control::rules::value_type const& rule) {
        auto const& [id, attributes] = rule;

        auto const rule_matched_filename = attributes.matched_files.matches(filename);
        auto const rule_ignored_filename = attributes.ignored_files.matches(filename);

        auto file_is_relevant = rule_matched_filename and not rule_ignored_filename;
        auto line_is_relevant = std::function<bool(int)>{ [](auto) { return true; } };

        if (file_is_relevant) {
          switch (container::value_or_default(shared_workflow.get(), attributes.type).check) {
          case control::check::NO_LINES:
            [[fallthrough]];
          case control::check::NO_FILES:
            file_is_relevant = false;
            break;
          case control::check::CHANGED_LINES:
            line_is_relevant = [is_modified = shared_file_changes.get()](auto line) { return is_modified[line]; };
            [[fallthrough]];
          case control::check::CHANGED_FILES:
            file_is_relevant = not shared_file_changes.get().empty();
            break;
          case control::check::ALL_LINES:
            [[fallthrough]];
          case control::check::ALL_FILES:
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

  // FIXME: eine print() funktion für eine header und für eine match klasse anbieten
  // namespace output::print()

  template <class FUNCTION>
  auto print_matches(std::ostream&                          output,
                     control::rules::value_type const&      rule,
                     std::shared_future<std::string> const& shared_content,
                     FUNCTION                               is_relevant) {
    auto const& [id, attributes] = rule;

    auto search = text::forward_search{ shared_content.get() };

    while (search.next(attributes.matched_text)) {
      auto const matched_lines = search.matched_lines();
      if (attributes.ignored_text.matches(matched_lines))
        continue;

      auto const line_number = search.line() + 1;
      if (not is_relevant(line_number))
        continue;

      auto const marked_text  = search_marked_text(search.matched_text(), attributes.marked_text);
      auto const highlighting = excerpt{ matched_lines, marked_text };

      format::print(output, "# line {}\n{}FEEDBACK_MATCH_{}(\"{}\", \"{}\")\n", line_number, highlighting.indentation,
                    format::uppercase{ id }, format::as_literal{ highlighting.first_line },
                    highlighting.indentation + highlighting.annotation);
    }
  }

  template <class FUNCTION>
  void print_matches(std::ostream&                             output,
                     std::shared_future<control::rules> const& shared_rules,
                     std::shared_future<std::string> const&    shared_content,
                     FUNCTION                                  is_relevant_in_file) {
    auto const& rules = shared_rules.get();

    std::for_each(std::execution::par, cbegin(rules), cend(rules), [=, &output](auto const& rule) {
      auto const is_rule_in_file_relevant = is_relevant_in_file(rule);
      if (not is_rule_in_file_relevant())
        return;

      auto synched_output = cxx20::osyncstream{ output };
      print_matches(synched_output, rule, shared_content, is_rule_in_file_relevant);
    });
  }

  void print_matches(std::ostream&                                       output,
                     std::shared_future<control::rules> const&           shared_rules,
                     std::shared_future<control::workflow> const&        shared_workflow,
                     std::shared_future<std::vector<std::string>> const& shared_sources,
                     std::shared_future<scm::diff> const&                shared_diff) {
    auto const  is_relevant = is_relevant_function(shared_workflow, shared_diff);
    auto const& sources     = shared_sources.get();

    std::for_each(std::execution::par, cbegin(sources), cend(sources), [=, &output](std::string const& filename) {
      auto const shared_content = async::share([=] { return content(filename); });

      auto synched_output = cxx20::osyncstream{ output };
      format::print(synched_output, "\n# line 1 \"{}\"\n", filename);

      auto const is_file_relevant = is_relevant(filename);
      print_matches(synched_output, shared_rules, shared_content, is_file_relevant);
    });
  }

  auto parse_sources(std::string const& filename) -> std::vector<std::string> {
    auto sources = std::vector<std::string>{};
    auto content = std::ifstream{ filename };

    for (std::string source; std::getline(content, source);)
      sources.emplace_back(source);

    return sources;
  }

  auto parse_diff_async(std::string const& filename) {
    return async::share([=] { return scm::diff::parse(content(filename)); });
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
} // namespace feedback

int main(int argc, char* argv[]) {
  std::ios::sync_with_stdio(false);

  std::stringstream out;

  try {
    auto const parameters = generator::cli::parse(argc, argv);

    auto const shared_diff     = feedback::parse_diff_async(parameters.diff_filename);
    auto const shared_rules    = feedback::parse_rules_async(parameters.rules_filename);
    auto const shared_workflow = feedback::parse_workflow_async(parameters.workflow_filename);
    auto const shared_sources  = feedback::parse_sources_async(parameters.sources_filename);

    print(out, feedback::output::header{ shared_rules, shared_workflow, parameters.rules_filename });
    feedback::print_matches(out, shared_rules, shared_workflow, shared_sources, shared_diff);
  }
  catch (std::exception const& e) {
    std::cerr << e.what() << '\n';
    return 1;
  }

  std::cout << out.rdbuf();
  return 0;
}
