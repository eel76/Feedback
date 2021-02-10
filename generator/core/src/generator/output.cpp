#include "generator/output.h"

#include "generator/container.h"
#include "generator/format.h"
#include "generator/future.h"
#include "generator/io.h"
#include "generator/json.h"
#include "generator/text.h"

#include <cxx20/syncstream>

#include <algorithm>
#include <execution>
#include <fstream>
#include <future>
#include <ostream>

using fmt::operator""_a;

namespace generator::output {

  template <class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
  template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

  auto make_relevant_matches(std::shared_future<feedback::workflow> const& shared_workflow,
                             std::shared_future<scm::diff> const&          shared_diff) {
    return [=](std::string_view source) {
      auto const shared_source_changes = future::launch_async([=] { return shared_diff.get().changes_from(source); }).share();

      return [=](feedback::rules::value_type const& rule) {
        auto const& [id, attributes] = rule;

        auto const rule_matched_filename = attributes.matched_files.matches(source);
        auto const rule_ignored_filename = attributes.ignored_files.matches(source);

        auto file_is_relevant = rule_matched_filename and not rule_ignored_filename;
        auto line_is_relevant = std::function<bool(int)>{ [](auto) { return true; } };

        if (file_is_relevant) {
          auto const& workflow = shared_workflow.get();

          switch (workflow[attributes.type].check) {
          case feedback::check::NO_LINES:
            [[fallthrough]];
          case feedback::check::NO_FILES:
            file_is_relevant = false;
            break;
          case feedback::check::CHANGED_LINES:
            line_is_relevant = [is_modified = shared_source_changes.get()](auto line) { return is_modified[line]; };
            [[fallthrough]];
          case feedback::check::CHANGED_FILES:
            file_is_relevant = not shared_source_changes.get().empty();
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

  struct header {
    std::string_view const&                       rules_origin;
    std::shared_future<feedback::rules> const&    shared_rules;
    std::shared_future<feedback::workflow> const& shared_workflow;
  };

  struct source {
    std::string const& filename;
  };

  struct match {
    std::string_view const& id;
    int const&              line_number;
    text::excerpt const&    highlighting;
  };

  struct rule_in_source_matches {
    std::string_view const&                       rule_origin;
    feedback::rules::value_type const&            rule;
    std::shared_future<std::string> const&        shared_source;
    std::shared_future<feedback::workflow> const& shared_workflow;
  };

  struct source_matches {
    std::string_view const&                       rules_origin;
    std::shared_future<feedback::rules> const&    shared_rules;
    std::shared_future<std::string> const&        shared_source;
    std::shared_future<feedback::workflow> const& shared_workflow;
  };

  void print(std::ostream& out, output::header header) {
    format::print(out,
                  R"_(// DO NOT EDIT: this file is generated automatically

namespace {{ using dummy = int; }}

#define __STRINGIFY(x) #x
#define STRINGIFY(x)   __STRINGIFY(x)
#define PRAGMA(x)      _Pragma(#x)

#if defined __GNUC__
#define FEEDBACK_RESPONSE_ERROR(id, msg)   PRAGMA(GCC error "feedback " STRINGIFY(id) ": " msg)
#define FEEDBACK_RESPONSE_WARNING(id, msg) PRAGMA(GCC warning "feedback " STRINGIFY(id) ": " msg)
#define FEEDBACK_RESPONSE_MESSAGE(id, msg) PRAGMA(message "feedback " STRINGIFY(id) ": " msg)
#define FEEDBACK_RESPONSE_NONE(id, msg)    /* no feedback response for id */
#elif defined _MSC_VER
#define FEEDBACK_MESSAGE(msg)              PRAGMA(message(__FILE__ "(" STRINGIFY(__LINE__) "): " msg))
#define FEEDBACK_RESPONSE_ERROR(id, msg)   FEEDBACK_MESSAGE("feedback error " STRINGIFY(id) ": " msg)
#define FEEDBACK_RESPONSE_WARNING(id, msg) FEEDBACK_MESSAGE("feedback warning " STRINGIFY(id) ": " msg)
#define FEEDBACK_RESPONSE_MESSAGE(id, msg) FEEDBACK_MESSAGE("feedback message " STRINGIFY(id) ": " msg)
#define FEEDBACK_RESPONSE_NONE(id, msg)    /* no feedback response for id */
#else
#error "Unsupported compiler"
#endif

)_");

    auto const& workflow = header.shared_workflow.get();

    for (auto [id, rule] : header.shared_rules.get())
      format::print(out,
                    R"_(#define FEEDBACK_MATCH_{uppercase_id}(match, highlighting) FEEDBACK_RESPONSE_{response}({id}, "{summary} [{type} from file://{origin}]\n |\n | " match "\n | " highlighting "\n |\n | RATIONALE : {rationale}\n | WORKAROUND: {workaround}\n |")
)_",
                    "origin"_a = format::as_literal{ header.rules_origin }, "id"_a = id,
                    "uppercase_id"_a = format::uppercase{ id },
                    "response"_a     = format::uppercase{ json::to_string(workflow[rule.type].response) },
                    "type"_a = format::as_literal{ rule.type }, "summary"_a = format::as_literal{ rule.summary },
                    "rationale"_a = format::as_literal{ rule.rationale }, "workaround"_a = format::as_literal{ rule.workaround });
  }

  void print(std::ostream& out, output::source source) {
    format::print(out, "\n# line 1 \"{}\"\n", source.filename);
  }

  void print(std::ostream& out, output::match match) {
    format::print(out, "# line {}\n{}FEEDBACK_MATCH_{}(\"{}\", \"{}\")\n", match.line_number, match.highlighting.indentation,
                  format::uppercase{ match.id }, format::as_literal{ match.highlighting.first_line },
                  match.highlighting.indentation + match.highlighting.annotation);
  }

  template <class FUNCTION>
  auto print(std::ostream& out, rule_in_source_matches matches, FUNCTION relevant_rule_in_source_matches) {
    auto const& [id, attributes] = matches.rule;

    auto search = text::forward_search{ matches.shared_source.get() };

    while (search.next_but(attributes.matched_text, attributes.ignored_text)) {
      auto const line_number = search.line();
      if (not relevant_rule_in_source_matches(line_number))
        continue;

      print(out, output::match{ id, line_number, search.highlighted_text(attributes.marked_text) });
    }
  }

  template <class FUNCTION> void print(std::ostream& out, source_matches matches, FUNCTION relevant_source_matches) {
    auto const& rules = matches.shared_rules.get();

    std::for_each(std::execution::par, cbegin(rules), cend(rules), [=, &out](auto const& rule) {
      auto const relevant_rule_in_source_matches = relevant_source_matches(rule);
      if (not relevant_rule_in_source_matches())
        return;

      auto synchronized_out = cxx20::osyncstream{ out };

      print(synchronized_out, rule_in_source_matches{ matches.rules_origin, rule, matches.shared_source, matches.shared_workflow },
            relevant_rule_in_source_matches);
    });
  }

  void print(std::ostream& out, output::matches matches) {
    print(out, header{ matches.rules_origin, matches.shared_rules, matches.shared_workflow });

    auto const  relevant_matches = make_relevant_matches(matches.shared_workflow, matches.shared_diff);
    auto const& sources          = matches.shared_sources.get();

    std::for_each(std::execution::par, cbegin(sources), cend(sources), [=, &out](std::string const& source) {
      auto const shared_source = future::launch_async([=] { return io::content(source); }).share();

      auto synchronized_out = cxx20::osyncstream{ out };

      print(synchronized_out, output::source{ source });
      print(synchronized_out, source_matches{ matches.rules_origin, matches.shared_rules, shared_source, matches.shared_workflow },
            relevant_matches(source));
    });
  }
} // namespace generator::output
