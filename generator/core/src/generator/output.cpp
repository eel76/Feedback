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
                    "origin"_a = format::as_literal{ header.rules_origin.generic_u8string() }, "id"_a = id,
                    "uppercase_id"_a = format::uppercase{ id },
                    "response"_a     = format::uppercase{ to_string(workflow[rule.type].response) },
                    "type"_a = format::as_literal{ rule.type }, "summary"_a = format::as_literal{ rule.summary },
                    "rationale"_a = format::as_literal{ rule.rationale }, "workaround"_a = format::as_literal{ rule.workaround });
  }

  void print(std::ostream& out, output::source source) {
    format::print(out, "\n#line 1 \"{}\"\n", source.filename.generic_u8string());
  }

  void print(std::ostream& out, output::match match) {
    auto const& [id, attributes] = match.rule;

    format::print(out,
                  R"_(#if defined __GNUC__
# line {line}
# pragma GCC warning \
{indentation}"feedback {id}: {summary} [ {type} from file://{origin} ]\n      | > rationale  > {rationale}\n      | > workaround > {workaround}\n      |"
#elif defined _MSC_VER
# line {line}

{indentation}FEEDBACK_MATCH_{ID}("{match}", "{highlighting}")
#endif
)_",
                  "line"_a = match.line_number - 1, "indentation"_a = match.highlighting.indentation,
                  "origin"_a = match.rules_origin.generic_u8string(), "id"_a = id, "ID"_a = format::uppercase{ id },
                  "type"_a = format::as_literal{ attributes.type }, "summary"_a = format::as_literal{ attributes.summary },
                  "rationale"_a    = format::as_literal{ attributes.rationale },
                  "workaround"_a   = format::as_literal{ attributes.workaround },
                  "match"_a        = format::as_literal{ match.highlighting.first_line },
                  "highlighting"_a = match.highlighting.indentation + match.highlighting.annotation);
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

      // compiler.emit_feedback (response, ...)
      print(out, output::match{ matches.rules_origin, matches.rule, line_number,
                                search.highlighted_text(attributes.marked_text), matches.shared_workflow });
    }
  }

  template <class FUNCTION>
  // emit (compiler, relevant_source_matches)
  void print(std::ostream& out, source_matches matches, FUNCTION relevant_source_matches) {
    auto const& rules = matches.shared_rules.get();

    std::for_each(std::execution::par, cbegin(rules), cend(rules), [=, &out](auto const& rule) {
      auto const relevant_rule_in_source_matches = relevant_source_matches(rule);
      if (not relevant_rule_in_source_matches())
        return;

      // compiler.share ()
      auto synchronized_out = cxx20::osyncstream{ out };

      print(synchronized_out, rule_in_source_matches{ matches.rules_origin, rule, matches.shared_source, matches.shared_workflow },
            relevant_rule_in_source_matches);
    });
  }

  // emit (compiler, matches)
  void print(std::ostream& out, output::matches matches) {
    // compiler.emit_header (header{})
    print(out, header{ matches.rules_origin, matches.shared_rules, matches.shared_workflow });

    auto const  relevant_matches = make_relevant_matches(matches.shared_workflow, matches.shared_diff);
    auto const& sources          = matches.shared_sources.get();

    std::for_each(std::execution::par, cbegin(sources), cend(sources), [=, &out](std::filesystem::path const& source) {
      auto const shared_source = std::async(std::launch::async, [=] { return io::content(source); }).share();

      // auto local_compiler = compiler.share ().source_scope (source)
      auto synchronized_out = cxx20::osyncstream{ out };

      print(synchronized_out, output::source{ source });
      print(synchronized_out, source_matches{ matches.rules_origin, matches.shared_rules, shared_source, matches.shared_workflow },
            relevant_matches(source));
    });
  }
} // namespace generator::output
