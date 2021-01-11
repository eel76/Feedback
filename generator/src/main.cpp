#include "feedback/async.h"
#include "feedback/container.h"
#include "feedback/format.h"
#include "feedback/io.h"
#include "feedback/parameter.h"
#include "feedback/regex.h"
#include "feedback/text.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <charconv>
#include <filesystem>
#include <cxx20/syncstream>
#include <map>
#include <optional>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>

namespace feedback {

    template <class T>
    auto to_string(T value)-> typename std::enable_if<std::is_enum_v<T>, std::string>::type
    {
      auto const json_dump = nlohmann::json{ value }.dump();
      return json_dump.substr(2, json_dump.length() - 4);
    }

  enum class response
  {
    WARNING,
    ERROR
  };

  NLOHMANN_JSON_SERIALIZE_ENUM(response, {
      {response::WARNING, "warning"},
      {response::ERROR, "error"}
    })


  enum class check
  {
    ALL_FILES,
    ALL_CODE_LINES,
    CHANGED_FILES,
    CHANGED_CODE_LINES,
    NO_FILES,
    NO_CODE_LINES
  };

  NLOHMANN_JSON_SERIALIZE_ENUM(check, {
      {check::ALL_FILES, "all_files"},
      {check::ALL_CODE_LINES, "all_code_lines"},
      {check::CHANGED_FILES, "changed_files"},
      {check::CHANGED_CODE_LINES, "changed_code_lines"},
      {check::NO_FILES, "no_files"},
      {check::NO_CODE_LINES, "no_code_lines"}
    })

  struct handling
  {
    feedback::check check;
    feedback::response response;
  };

  void from_json(nlohmann::json const& json, handling& handling)
  {
    handling.check = json.at("check");
    handling.response = json.at("response");
  }

  template <class C>
  class json_container : public C
  {
  public:
    static auto parse_from(std::string const& filename)
    {
      std::ifstream input_stream{ filename };

      auto json = nlohmann::json{};
      input_stream >> json;

      auto ret = json.get<json_container<C>>();
      ret.origin_ = filename;

      return ret;
    }

    auto origin() const
    {
      return origin_;
    }

  private:
    std::string origin_;
  };

  using workflow = json_container<std::map<std::string, handling>>;

  struct rule
  {
    std::string type;
    std::string summary;
    std::string rationale;
    std::string workaround;
    regex::precompiled matched_files;
    regex::precompiled ignored_files;
    regex::precompiled matched_text;
    regex::precompiled ignored_text;
    regex::precompiled marked_text;
  };

  void from_json(nlohmann::json const& json, feedback::rule& rule)
  {
    rule.type = json.at("type");
    rule.summary = json.at("summary");
    rule.rationale = json.at("rationale");
    rule.workaround = json.at("workaround");
    rule.matched_files = regex::compile(regex::capture(json.at("matched_files").get<std::string>()));
    rule.ignored_files = regex::compile(regex::capture(json.at("ignored_files").get<std::string>()));
    rule.matched_text = regex::compile(regex::capture(json.at("matched_text").get<std::string>()));
    rule.ignored_text = regex::compile(regex::capture(json.at("ignored_text").get<std::string>()));
    rule.marked_text = regex::compile(regex::capture(json.at("marked_text").get<std::string>()));
  }

  class identifier
  {
  public:
    explicit identifier(std::string const& str)
      : origin(str)
      , prefix(origin)
      , nr()
    {
      auto nr_match = std::string_view{};

      auto const pattern = regex::compile(R"(^([^\d]+)(\d\d?\d?\d?\d?)$)");

      if (not pattern.matches(origin, { &prefix, &nr_match }))
        return;

      nr = std::stoi(std::string{ nr_match });
    }

    operator std::string_view() const
    {
      return origin;
    }

    bool operator<(identifier const& other) const
    {
      if (auto const prefix_comparision = prefix.compare(other.prefix); prefix_comparision != 0)
        return prefix_comparision < 0;

      return nr < other.nr;
    }

  private:
    std::string origin;
    std::string_view prefix;
    std::optional<int> nr;
  };

  using rules = json_container<std::map<identifier, rule>>;

} // namespace feedback

namespace {

  auto async_content_from(std::string const& filename)
  {
    return async::launch([=] {
      auto input_stream = std::ifstream{ filename };
      return io::content(input_stream);
      })
      .share();
  }

  auto search_marked_text(std::string_view const& text, regex::precompiled const& pattern)
  {
    auto search = text::forward_search{ text };

    if (search.next(pattern))
      return search.matched_text();

    return text;
  }

  class excerpt
  {
  public:
    excerpt(std::string_view text, std::string_view match)
    {
      assert(text.data() <= match.data());
      assert(text.data() + text.length() >= match.data() + match.length());

      first_line = text::first_line_of(text);

      indentation.append(match.data() - text.data(), ' ');
      annotation.append(text::first_line_of(match).length(), '~');

      if (!annotation.empty())
        annotation[0] = '^';
    }

  public:
    std::string_view first_line;
    std::string indentation;
    std::string annotation;
  };

  auto make_check_rule_in_file_function(std::string const& filename)
  {
    return[filename,
      file_content = async_content_from(filename)](auto const& id, auto const& rule) {

      if (not rule.matched_files.matches(filename))
        return fmt::format("\n// rule {} not matched for current file name", id);

      if (rule.ignored_files.matches(filename))
        return fmt::format("\n// rule {} ignored for current file name", id);

      auto out = std::ostringstream{};

      auto search = text::forward_search{ file_content.get() };
      while (search.next(rule.matched_text))
      {
        auto const matched_lines = search.matched_lines();

        if (rule.ignored_text.matches(matched_lines))
        {
          format::print(out, "\n// ignored match in line {}", search.line());
          continue;
        }

        // rule.type im workflow nachschauen, was wird gecheckt
        // no_files => ignorieren
        // all_files reporten
        // changed_files => im diff überprüfen
        // changed_code_lines => im diff überprüfen 

        auto const marked_text = search_marked_text(search.matched_text(), rule.marked_text);
        auto const highlighting = excerpt{ matched_lines, marked_text };

        format::print(
          out,
          "\n# line {}\n{}FEEDBACK_{}_MATCH(\"{}\", \"{}\")",
search.line() + 1,
highlighting.indentation,
format::uppercase{ id },
format::as_literal{ highlighting.first_line },
highlighting.indentation + highlighting.annotation);
      }

      return out.str();
    };
  }

  auto async_messages_from(feedback::rules const& rules, std::string const& source_filename)
  {
    auto const check_rule_in_file_function = make_check_rule_in_file_function(source_filename);

    auto messages = std::vector<std::future<std::string>>{};

    for (auto const& [id, rule] : rules)
      messages.push_back(async::launch(check_rule_in_file_function, id, rule));

    return messages;
  }

  auto async_rules_from(std::string const& filename)
  {
    return async::launch([filename] {
      return feedback::rules::parse_from(filename);
      })
      .share();
  }

  using filter = std::function<bool(std::string const&, int)>;

  auto make_check_rules_function(std::shared_future<feedback::rules> rules)
  {
    return [rules](std::string const& filename) {

      auto messages = async_messages_from(rules.get(), filename);

      auto synchronized_out = cxx20::osyncstream{ std::cout };
      format::print(synchronized_out, "\n\n# line 1 \"{}\"", filename);

      for (auto&& message : messages)
        synchronized_out << message.get();
    };
  }

  auto async_workflow_from(std::string const& filename) -> std::shared_future<feedback::workflow>
  {
    return async::launch([filename] {
      // FIXME: what does a missing workflow mean?
      if (filename.empty())
        return feedback::workflow{};

      return feedback::workflow::parse_from(filename);
      })
      .share();
  }

  void print_header(feedback::rules const& rules, feedback::workflow const& workflow)
  {
    using fmt::operator""_a;

    format::print(
      std::cout,
      R"_(// DO NOT EDIT: this file is generated automatically

namespace {{ using avoid_compiler_warnings_symbol = int; }}

#define FEEDBACK_RULES "file://{feedback_rules}"
#define FEEDBACK_WORKFLOW "file://{feedback_workflow}"

#define __STRINGIFY(x) #x
#define STRINGIFY(x) __STRINGIFY(x)
#define PRAGMA(x) _Pragma (#x)

#if defined (__GNUC__)
# define FEEDBACK_ERROR_RESPONSE(id, msg) PRAGMA(GCC error STRINGIFY(id) ": " msg)
# define FEEDBACK_WARNING_RESPONSE(id, msg) PRAGMA(GCC warning STRINGIFY(id) ": " msg)
#else
# define FEEDBACK_ERROR_RESPONSE(id, msg) PRAGMA(message (__FILE__ "(" STRINGIFY(__LINE__) "): error " STRINGIFY(id) ": " msg))
# define FEEDBACK_WARNING_RESPONSE(id, msg) PRAGMA(message (__FILE__ "(" STRINGIFY(__LINE__) "): warning " STRINGIFY(id) ": " msg))
#endif
)_"
, "feedback_rules"_a = format::as_literal{ rules.origin() }
, "feedback_workflow"_a = format::as_literal{ workflow.origin() });

    for (auto [id, rule] : rules)
      format::print(
        std::cout,
        R"_(
#define FEEDBACK_{uppercase_id}_MATCH(match, highlighting) FEEDBACK_{response}_RESPONSE({id}, "{summary} [{type} from " FEEDBACK_RULES "]\n |\n | " match "\n | " highlighting "\n |\n | RATIONALE : {rationale}\n | WORKAROUND: {workaround}\n |"))_",
"id"_a = id,
"uppercase_id"_a = format::uppercase{ id },
"response"_a = format::uppercase{ to_string(workflow.at(rule.type).response) },
"type"_a = format::as_literal{ rule.type },
"summary"_a = format::as_literal{ rule.summary },
"rationale"_a = format::as_literal{ rule.rationale },
"workaround"_a = format::as_literal{ rule.workaround });
  }

  void print_matches(std::string const& source_files, std::function<void(std::string const&)> print_messages_from)
  {
    auto tasks = std::vector<async::task>{};

    auto content = std::ifstream{ source_files };
    for (std::string source_file; std::getline(content, source_file);)
      tasks.emplace_back([=] { print_messages_from(source_file); });
  }

  using changed_lines = container::interval_map<int, bool>;
  using changed_files = std::unordered_map<std::string, changed_lines>;

  auto parse_diff_block(std::string_view diff_block, changed_lines changes = changed_lines{ false })->changed_lines
  {
    auto starting_line_pattern = regex::compile("@@ [-][,0-9]+ [+]([0-9]+)[, ].*@@");
    auto line_pattern = regex::compile("\n([+ ])");

    regex::match starting_line;
    if (!starting_line_pattern.matches(diff_block, { &starting_line }))
      return changes;

    int line_number = 1;

    auto [p, ec] = std::from_chars(starting_line.data(), starting_line.data() + starting_line.size(), line_number);
    if (ec == std::errc::invalid_argument)
      return changes;

    auto line_search = text::forward_search{ diff_block };
    while (line_search.next(line_pattern))
    {
      auto const line = line_search.matched_text();

      if (line[0] == '+')
        changes.assign(line_number, line_number + 1, true);

      ++line_number;
    }

    return changes;
  }

  auto parse_diff_section(std::string_view diff_section, changed_files changes = {}) -> changed_files
  {
    auto filename_pattern = regex::compile("\n[-][-][-] a/.+\n[+][+][+] b/(.+)\n");
    auto block_pattern = regex::compile("\n(@@ [-][,0-9]+ [+][,0-9]+ @@.*\n([-+ ].*\n)*)");

    regex::match filename_match;
    if (!filename_pattern.matches(diff_section, { &filename_match }))
      return changes;

    auto const filename = std::string{ filename_match };
    changes.try_emplace(filename, changed_lines{ false });

    auto file_changes = std::move (changes.at(filename));

    auto search = text::forward_search{ diff_section };
    while (search.next(block_pattern))
    {
      auto const block = search.matched_text();
      file_changes = parse_diff_block(block, std::move(file_changes));
    }

    changes.at (filename) = std::move (file_changes);
    return changes;
  }

  auto parse_diff(std::string_view diff, changed_files changes = {}) -> changed_files
  {
    // FIXME: debug me: auto const section_pattern = regex::compile("(?:^|\n)([a-z].*\n)+([-][-][-] a/.+\n[+][+][+] b/(.+)\n([-+ @].*\n)*)");
    auto const section_pattern = regex::compile("(?:^|\n)((?:[a-z].*\n)+[-][-][-] a/.+\n[+][+][+] b/(.+)\n([-+ @].*\n)*)");

    auto search = text::forward_search{ diff };
    while (search.next(section_pattern))
    {
      auto const section = search.matched_text();
      changes = parse_diff_section(section, std::move (changes));
    }

    return changes;
  }

  auto async_changes_from(std::string const& diff_filename) -> std::shared_future<changed_files>
  {
    return async::launch([diff_filename] {
      if (diff_filename.empty())
        return changed_files{};

      return parse_diff(async_content_from(diff_filename).get());
      })
      .share();
  }
} // namespace

// make_filter (rule, workflow, changes)

// filter (filename)
// filter (filename, linenr)
// make a filter for each rule 
int main(int argc, char* argv[])
{
  std::ios::sync_with_stdio(false);

  try
  {
    auto const parameters = parameter::parse(argc, argv);

    auto const rules = async_rules_from(parameters.rules_filename);
    auto const workflow = async_workflow_from(parameters.workflow_filename);
    auto const diff = async_changes_from(parameters.diff_filename);
    auto const check_rules_function = make_check_rules_function(rules);

    auto const redirected_output = io::redirect(std::cout, parameters.output_filename);

    print_header(rules.get(), workflow.get());
    print_matches(parameters.sources_filename, check_rules_function);
  }
  catch (std::exception const& e)
  {
    std::cerr << e.what() << '\n';
    return 1;
  }

  return 0;
}
