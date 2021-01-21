#include "feedback/async.h"
#include "feedback/format.h"
#include "feedback/regex.h"
#include "feedback/scm.h"
#include "feedback/stream.h"
#include "feedback/text.h"

#include "generator/cli.h"

#include <cxx20/syncstream>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <execution>
#include <filesystem>
#include <map>
#include <optional>
#include <type_traits>

namespace feedback {

  namespace {
    template <class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
    template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

    auto content_from(std::string const& filename) -> std::string {
      if (filename.empty())
        return {};

      auto input_stream = std::ifstream{ filename };
      return stream::content(input_stream);
    }
  } // namespace

  template <class T> auto to_string(T value) -> typename std::enable_if<std::is_enum_v<T>, std::string>::type {
    auto const json_dump = nlohmann::json{ value }.dump();
    return json_dump.substr(2, json_dump.length() - 4);
  }

  enum class response { NONE, MESSAGE, WARNING, ERROR };

  NLOHMANN_JSON_SERIALIZE_ENUM(response,
                               { { response::NONE, "none" },
                                 { response::MESSAGE, "message" },
                                 { response::WARNING, "warning" },
                                 { response::ERROR, "error" } })

  enum class check { ALL_FILES, ALL_LINES, CHANGED_FILES, CHANGED_LINES, NO_FILES, NO_LINES };

  NLOHMANN_JSON_SERIALIZE_ENUM(check,
                               { { check::ALL_FILES, "all_files" },
                                 { check::ALL_LINES, "all_lines" },
                                 { check::CHANGED_FILES, "changed_files" },
                                 { check::CHANGED_LINES, "changed_lines" },
                                 { check::NO_FILES, "no_files" },
                                 { check::NO_LINES, "no_lines" } })

  struct handling {
    feedback::check    check;
    feedback::response response;
  };

  void from_json(nlohmann::json const& json, handling& handling) {
    handling.check    = json.value("check", check::ALL_FILES);
    handling.response = json.value("response", response::MESSAGE);
  }

  template <class C> class json_container : public C {
  public:
    static auto parse_from(std::string const& filename) -> json_container<C> {
      auto const json = nlohmann::json::parse(content_from(filename));

      auto ret    = json.get<json_container<C>>();
      ret.origin_ = filename;

      return ret;
    }

    auto origin() const {
      return origin_;
    }

  private:
    std::string origin_;
  };

  using workflow = json_container<std::map<std::string, handling>>;

  struct rule {
    std::string        type;
    std::string        summary;
    std::string        rationale;
    std::string        workaround;
    regex::precompiled matched_files;
    regex::precompiled ignored_files;
    regex::precompiled matched_text;
    regex::precompiled ignored_text;
    regex::precompiled marked_text;
  };

  void from_json(nlohmann::json const& json, feedback::rule& rule) {
    rule.type          = json.at("type");
    rule.summary       = json.at("summary");
    rule.rationale     = json.value("rationale", "N/A");
    rule.workaround    = json.value("workaround", "N/A");
    rule.matched_files = regex::compile(regex::capture(json.value("matched_files", ".*")));
    rule.ignored_files = regex::compile(regex::capture(json.value("ignored_files", "^$")));
    rule.matched_text  = regex::compile(regex::capture(json.at("matched_text").get<std::string>()));
    rule.ignored_text  = regex::compile(regex::capture(json.value("ignored_text", "^$")));
    rule.marked_text   = regex::compile(regex::capture(json.value("marked_text", ".*")));
  }

  class identifier {
  public:
    explicit identifier(std::string const& str) : origin(str), prefix(origin), nr() {
      auto nr_match = std::string_view{};

      auto const pattern = regex::compile(R"(^([^\d]+)(\d\d?\d?\d?\d?)$)");

      if (not pattern.matches(origin, { &prefix, &nr_match }))
        return;

      nr = std::stoi(std::string{ nr_match });
    }

    operator std::string_view() const {
      return origin;
    }

    bool operator<(identifier const& other) const {
      if (auto const prefix_comparision = prefix.compare(other.prefix); prefix_comparision != 0)
        return prefix_comparision < 0;

      return nr < other.nr;
    }

  private:
    std::string        origin;
    std::string_view   prefix;
    std::optional<int> nr;
  };

  using rules = json_container<std::map<identifier, rule>>;

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
          switch (shared_workflow.get().at(attributes.type).check) {
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

  void print_header(std::ostream&                                 output,
                    std::shared_future<feedback::rules> const&    shared_rules,
                    std::shared_future<feedback::workflow> const& shared_workflow) {
    using fmt::operator""_a;

    format::print(output,
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

    auto const& rules    = shared_rules.get();
    auto const& workflow = shared_workflow.get();

    for (auto [id, rule] : rules)
      format::print(output,
                    R"_(#define FEEDBACK_MATCH_{uppercase_id}(match, highlighting) FEEDBACK_RESPONSE_{response}({id}, "{summary} [{type} from file://{feedback_rules}]\n |\n | " match "\n | " highlighting "\n |\n | RATIONALE : {rationale}\n | WORKAROUND: {workaround}\n |")
)_",
                    "feedback_rules"_a = format::as_literal{ rules.origin() }, "id"_a = id,
                    "uppercase_id"_a = format::uppercase{ id },
                    "response"_a     = format::uppercase{ to_string(workflow.at(rule.type).response) },
                    "type"_a = format::as_literal{ rule.type }, "summary"_a = format::as_literal{ rule.summary },
                    "rationale"_a = format::as_literal{ rule.rationale }, "workaround"_a = format::as_literal{ rule.workaround });
  }

  template <class FUNCTION>
  auto print_matches(std::ostream&                          output,
                     feedback::rules::value_type const&     rule,
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
  void print_matches(std::ostream&                              output,
                     std::shared_future<feedback::rules> const& shared_rules,
                     std::shared_future<std::string> const&     shared_content,
                     FUNCTION                                   is_relevant_in_file) {
    auto const& rules = shared_rules.get();

    std::for_each(std::execution::par, cbegin (rules), cend (rules), [=, &output](auto const& rule) {
      auto const is_rule_in_file_relevant = is_relevant_in_file(rule);
      if (not is_rule_in_file_relevant())
        return;

      auto synched_output = cxx20::osyncstream{ output };
      print_matches(synched_output, rule, shared_content, is_rule_in_file_relevant);
    });
  }

  void print_matches(std::ostream&                                       output,
                     std::shared_future<feedback::rules> const&          shared_rules,
                     std::shared_future<feedback::workflow> const&       shared_workflow,
                     std::shared_future<std::vector<std::string>> const& shared_sources,
                     std::shared_future<scm::diff> const&                shared_diff) {
    auto const  is_relevant = is_relevant_function(shared_workflow, shared_diff);
    auto const& sources     = shared_sources.get();

    std::for_each(std::execution::par, cbegin (sources), cend (sources), [=, &output](std::string const& filename) {
      auto const shared_content = async::share([=] { return content_from(filename); });

      auto synched_output = cxx20::osyncstream{ output };
      format::print(synched_output, "\n# line 1 \"{}\"\n", filename);

      auto const is_file_relevant = is_relevant(filename);
      print_matches(synched_output, shared_rules, shared_content, is_file_relevant);
    });
  }

  auto parse_diff_from(std::string const& filename) -> scm::diff {
    return scm::diff::parse_from(content_from(filename));
  }

  auto parse_sources_from(std::string const& filename) -> std::vector<std::string> {
    auto sources = std::vector<std::string>{};
    auto content = std::ifstream{ filename };

    for (std::string source; std::getline(content, source);)
      sources.emplace_back(source);

    return sources;
  }

  auto async_diff_from(std::string const& filename) {
    return async::share([=] { return parse_diff_from(filename); });
  }

  auto async_rules_from(std::string const& filename) {
    return async::share([=] { return feedback::rules::parse_from(filename); });
  }

  auto async_workflow_from(std::string const& filename) {
    return async::share([=] { return feedback::workflow::parse_from(filename); });
  }

  auto async_sources_from(std::string const& filename) {
    return async::share([=] { return parse_sources_from(filename); });
  }
} // namespace feedback

int main(int argc, char* argv[]) {
  std::ios::sync_with_stdio(false);

  try {
    auto const parameters = generator::cli::parse(argc, argv);

    auto const shared_rules    = feedback::async_rules_from(parameters.rules_filename);
    auto const shared_workflow = feedback::async_workflow_from(parameters.workflow_filename);
    auto const shared_sources  = feedback::async_sources_from(parameters.sources_filename);
    auto const shared_diff     = feedback::async_diff_from(parameters.diff_filename);

    auto const redirected_output = feedback::stream::redirect(std::cout, parameters.output_filename);

    print_header(std::cout, shared_rules, shared_workflow);
    print_matches(std::cout, shared_rules, shared_workflow, shared_sources, shared_diff);
  }
  catch (std::exception const& e) {
    std::cerr << e.what() << '\n';
    return 1;
  }

  return 0;
}
