#include "generator/output.h"

#include "generator/container.h"
#include "generator/format.h"
#include "generator/io.h"
#include "generator/text.h"

#include <cxx20/syncstream>

#include <algorithm>
#include <any>
#include <execution>
#include <fstream>
#include <future>
#include <iostream>
#include <ostream>

using fmt::operator""_a;

namespace generator::output {

  namespace {
    template <class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
    template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;
  } // namespace

  auto make_relevant_matches(std::shared_future<feedback::workflow> const& shared_workflow,
                             std::shared_future<scm::diff> const&          shared_diff) {
    return [=](std::filesystem::path const& source) {
      auto const shared_source_changes =
      std::async(std::launch::async, [=] { return shared_diff.get().changes_from(source); }).share();

      return [=](feedback::rules::value_type const& rule) {
        auto const& [id, attributes] = rule;

        auto const rule_matched_filename = attributes.matched_files.matches(source.generic_u8string());
        auto const rule_ignored_filename = attributes.ignored_files.matches(source.generic_u8string());

        auto file_is_relevant = rule_matched_filename and not rule_ignored_filename;
        auto line_is_relevant = std::function<bool(int)>{ [](auto) { return true; } };

        if (file_is_relevant) {
          auto const& workflow = shared_workflow.get();

          std::visit(overloaded{
                     [&](feedback::nothing) { file_is_relevant = false; },
                     [&](feedback::changed_lines) {
                       line_is_relevant = [is_modified = shared_source_changes.get()](auto line) {
                         return is_modified[line];
                       };
                       file_is_relevant = not shared_source_changes.get().empty();
                     },
                     [&](feedback::changed_files) { file_is_relevant = not shared_source_changes.get().empty(); },
                     [&](feedback::everything) {} },
                     workflow[attributes.type].check);

          if (std::holds_alternative<feedback::none>(workflow[attributes.type].response))
            file_is_relevant = false;
        }

        if (!file_is_relevant) {
          line_is_relevant = [](auto) { return false; };
        }

        return overloaded{ [=]() { return file_is_relevant; }, [=](auto line) { return line_is_relevant(line); } };
      };
    };
  }

  struct header {
    std::filesystem::path const&                  rules_origin;
    std::shared_future<feedback::rules> const&    shared_rules;
    std::shared_future<feedback::workflow> const& shared_workflow;
  };

  struct source {
    std::filesystem::path const& filename;
  };

  struct match {
    std::filesystem::path const&                  rules_origin;
    feedback::rules::value_type const&            rule;
    int const&                                    line_number;
    text::excerpt const&                          highlighting;
    std::shared_future<feedback::workflow> const& shared_workflow;
  };

  struct rule_in_source_matches {
    std::filesystem::path const&                  rules_origin;
    feedback::rules::value_type const&            rule;
    std::shared_future<std::string> const&        shared_source;
    std::shared_future<feedback::workflow> const& shared_workflow;
  };

  struct source_matches {
    std::filesystem::path const&                  rules_origin;
    std::shared_future<feedback::rules> const&    shared_rules;
    std::shared_future<std::string> const&        shared_source;
    std::shared_future<feedback::workflow> const& shared_workflow;
  };

  template <typename Interface> struct polymorphic_value {
  public:
    template <typename ConcreteType>
    explicit polymorphic_value(ConcreteType&& object)
    : storage{ std::forward<ConcreteType>(object) }, getter{ [](std::any* storage) -> Interface* {
        return std::any_cast<ConcreteType>(storage);
      } } {
    }

    auto operator->() -> Interface* {
      return getter(&storage);
    }
    auto operator->() const -> Interface const* {
      return getter(const_cast<std::any*>(&storage));
    }

  private:
    std::any storage;
    Interface* (*getter)(std::any*);
  };

  /*

  using compiler = polymorphic_value<compiler_interface>;


  std::vector<compiler> compilers;


  class compiler_interface
  {
  public:
    // return a printer which prints #line 1 filename and then #ifdef in constructor and #endif in destructor
    auto get_source_printer (std::ostream& out, filename) const -> source_printer;

    class source_printer
    {
    public:
      auto get_response_printer (response) const;
      void emit (response, match);
    };

  protected:
    virtual void emit_header (out) const = 0;
    virtual void emit_footer (out) const = 0;

    virtual void emit_message (out, match) const = 0;
    virtual void emit_warning (out, match) const = 0;
    virtual void emit_error (out, match) const = 0;
  };

  class msvc : public compiler_interface
  {
  };

  class gcc : public compiler_interface
  {
  };


  */

  void print(std::ostream& out, output::header) {
    format::print(out,
                  R"_(// DO NOT EDIT: this file is generated automatically

namespace {{ using dummy = int; }}

# define __STRINGIFY(x) #x
# define STRINGIFY(x)   __STRINGIFY(x)
# define PRAGMA(x)      _Pragma(#x)
# define MESSAGE(msg)   PRAGMA(message(__FILE__ "(" STRINGIFY(__LINE__) "): " msg))

)_");
  }

  void print(std::ostream& out, output::source source) {
    format::print(out, "\n#line 1 \"{}\"\n", source.filename.generic_u8string());
  }

  struct location {
    int line;
    int column;
  };

  struct message {
    output::location const& location;
    std::string_view const& text;
    text::excerpt const&    highlighting;
  };

  void print(std::ostream& out, output::message message) {
    format::print(out,
                  R"_(#if defined __GNUC__
# line {line_before}
# pragma message "{text}\n      |" \
{indentation}"                                                                                          "
#elif defined _MSC_VER
# line {line}
{indentation}MESSAGE("note {text}\n      |\n{line:>5} | {match}\n      | {indentation}{annotation}")
#endif
)_",
                  "line_before"_a = message.location.line - 1, "line"_a = message.location.line,
                  "match"_a       = format::as_compiler_message{ message.highlighting.first_line },
                  "text"_a        = format::as_compiler_message{ message.text },
                  "indentation"_a = message.highlighting.indentation, "annotation"_a = message.highlighting.annotation);
  }

  struct warning {
    output::location const& location;
    std::string_view const& text;
    text::excerpt const&    highlighting;
  };

  void print(std::ostream& out, output::warning warning) {
    format::print(out,
                  R"_(#if defined __GNUC__
# line {line_before}
# pragma GCC warning \
{indentation}"{text}\n      |                                                                           "
#elif defined _MSC_VER
# line {line}
{indentation}MESSAGE("warning {text}\n      |\n{line:>5} | {match}\n      | {indentation}{annotation}")
#endif
)_",
                  "line_before"_a = warning.location.line - 1, "line"_a = warning.location.line,
                  "match"_a       = format::as_compiler_message{ warning.highlighting.first_line },
                  "text"_a        = format::as_compiler_message{ warning.text },
                  "indentation"_a = warning.highlighting.indentation, "annotation"_a = warning.highlighting.annotation);
  }

  struct error {
    output::location const& location;
    std::string_view const& text;
    text::excerpt const&    highlighting;
  };

  void print(std::ostream& out, output::error error) {
    format::print(out,
                  R"_(#if defined __GNUC__
# line {line_before}
# pragma GCC error \
{indentation}"{text}\n      |                                                                           "
#elif defined _MSC_VER
# line {line}
{indentation}MESSAGE("error {text}\n      |\n{line:>5} | {match}\n      | {indentation}{annotation}")
#endif
)_",
                  "line_before"_a = error.location.line - 1, "line"_a = error.location.line,
                  "match"_a       = format::as_compiler_message{ error.highlighting.first_line },
                  "text"_a        = format::as_compiler_message{ error.text },
                  "indentation"_a = error.highlighting.indentation, "annotation"_a = error.highlighting.annotation);
  }

  template <class FUNCTION>
  // emit (compiler, relevant_rule_in_source_matches)

  auto print(std::ostream& out, rule_in_source_matches matches, FUNCTION relevant_rule_in_source_matches) {
    auto const& [id, attributes] = matches.rule;

    auto search = text::forward_search{ matches.shared_source.get() };

    while (search.next_but(attributes.matched_text, attributes.ignored_text)) {
      auto const line_number = search.line();
      if (not relevant_rule_in_source_matches(line_number))
        continue;

      auto const feedback = fmt::format("{id}: {summary} [ {type} from file://{origin} ]\nrationale  : "
                                        "{rationale}\nworkaround : {workaround}",
                                        "id"_a = id, "type"_a = attributes.type, "summary"_a = attributes.summary,
                                        "rationale"_a = attributes.rationale, "workaround"_a = attributes.workaround,
                                        "origin"_a = matches.rules_origin.generic_u8string());

      // compiler.emit_feedback (response, ...)

      auto const& workflow = matches.shared_workflow.get();
      std::visit(overloaded{ [&](feedback::none) {},
                             [&](feedback::message) {
                               print(out, output::message{ location{ line_number, search.column() }, feedback,
                                                           search.highlighted_text(attributes.marked_text) });
                             },
                             [&](feedback::warning) {
                               print(out, output::warning{ location{ line_number, search.column() }, feedback,
                                                           search.highlighted_text(attributes.marked_text) });
                             },
                             [&](feedback::error) {
                               print(out, output::error{ location{ line_number, search.column() }, feedback,
                                                         search.highlighted_text(attributes.marked_text) });
                             } },
                 workflow[attributes.type].response);
    }
  }

  template <class FUNCTION>
  // emit (compiler, relevant_source_matches)
  auto print(std::ostream& out, source_matches matches, FUNCTION relevant_source_matches, stats merged_stats = {}) {
    auto const& rules = matches.shared_rules.get();

    std::atomic_bool any_rule_relevant{ false };

    std::for_each(std::execution::par, cbegin(rules), cend(rules), [=, &out, &any_rule_relevant](auto const& rule) {
      auto const relevant_rule_in_source_matches = relevant_source_matches(rule);
      if (not relevant_rule_in_source_matches())
        return;

      any_rule_relevant = true;

      // compiler.share ()
      auto synchronized_out = cxx20::osyncstream{ out };

      print(synchronized_out, rule_in_source_matches{ matches.rules_origin, rule, matches.shared_source, matches.shared_workflow },
            relevant_rule_in_source_matches);
    });

    if (any_rule_relevant)
      merged_stats.process(matches.shared_source.get());

    return merged_stats;
  }

  // emit (compiler, matches)
  auto print(std::ostream& out, output::matches matches, stats merged_stats) -> stats {
    std::mutex lock;

    // compiler.emit_header (header{})
    print(out, header{ matches.rules_origin, matches.shared_rules, matches.shared_workflow });

    auto const  relevant_matches = make_relevant_matches(matches.shared_workflow, matches.shared_diff);
    auto const& sources          = matches.shared_sources.get();

    std::for_each(std::execution::par, cbegin(sources), cend(sources), [=, &out, &merged_stats, &lock](std::filesystem::path const& source) {
      auto const shared_source = std::async(std::launch::async, [=] { return io::content(source); }).share();

      // auto local_compiler = compiler.share ().source_scope (source)
      auto synchronized_out = cxx20::osyncstream{ out };

      print(synchronized_out, output::source{ source });
      auto const source_stats =
      print(synchronized_out, source_matches{ matches.rules_origin, matches.shared_rules, shared_source, matches.shared_workflow },
            relevant_matches(source));

      auto const locked = std::lock_guard(lock);
      merged_stats.merge(source_stats);
    });

    return merged_stats;
  }
} // namespace generator::output
