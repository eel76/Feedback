#include "feedback/async.h"
#include "feedback/container.h"
#include "feedback/format.h"
#include "feedback/stream.h"
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

  auto content_from(std::string const& filename) -> std::string
  {
    auto input_stream = std::ifstream{ filename };
    return stream::content(input_stream);
  }

  auto async_content_from(std::string const& filename) -> std::shared_future<std::string>
  {
    return async::share([=] { return content_from(filename); });
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

  template <class FILTER>
  auto find_matches_function(std::shared_future<std::string> shared_content, FILTER is_line_checked)
  {
    return[shared_content, is_line_checked](auto const& id, auto const& rule) -> std::string {

      auto out = std::ostringstream{};

      auto search = text::forward_search{ shared_content.get() };
      while (search.next(rule.matched_text))
      {
        auto const matched_lines = search.matched_lines();
        if (rule.ignored_text.matches(matched_lines))
          continue;

        auto const line_number = search.line() + 1;
        if (!is_line_checked(rule, line_number))
          continue;

        auto const marked_text = search_marked_text(search.matched_text(), rule.marked_text);
        auto const highlighting = excerpt{ matched_lines, marked_text };

        format::print(
          out,
          "\n# line {}\n{}FEEDBACK_{}_MATCH(\"{}\", \"{}\")",
          line_number,
          highlighting.indentation,
          format::uppercase{ id },
          format::as_literal{ highlighting.first_line },
          highlighting.indentation + highlighting.annotation);
      }

      return out.str();
    };
  }

  template <class FILTER>
  auto make_print_matches_function(std::shared_future<feedback::rules> shared_rules, FILTER is_line_in_file_checked)
  {
    return [shared_rules, is_line_in_file_checked](std::string const& filename) {

      auto const shared_content = async_content_from(filename);

      auto const is_line_checked = [filename, is_line_in_file_checked](feedback::rule const& rule, int line)
      {
        return is_line_in_file_checked(rule, filename, line);
      };

      auto const find_matches = find_matches_function(shared_content, is_line_checked);

      auto messages = std::vector<std::future<std::string>>{};

      for (auto const& [id, rule] : shared_rules.get())
      {
        if (not rule.matched_files.matches(filename))
          continue;

        if (rule.ignored_files.matches(filename))
          continue;

        messages.push_back(async::launch(find_matches, id, rule));
      }

      auto synchronized_out = cxx20::osyncstream{ std::cout };
      format::print(synchronized_out, "\n\n# line 1 \"{}\"", filename);

      for (auto&& message : messages)
        synchronized_out << message.get();
    };
  }

  auto async_rules_from(std::string const& filename)
  {
    return async::share([=] { return feedback::rules::parse_from(filename); });
  }

  auto async_workflow_from(std::string const& filename)
  {
    return async::share([=] {
      // FIXME: what does a missing workflow mean?
      if (filename.empty())
        return feedback::workflow{};

      return feedback::workflow::parse_from(filename);
      });
  }

  void print_header(std::shared_future<feedback::rules> const& shared_rules, std::shared_future<feedback::workflow> const& shared_workflow)
  {
    using fmt::operator""_a;

    format::print(
      std::cout,
      R"_(// DO NOT EDIT: this file is generated automatically

namespace {{ using avoid_compiler_warnings_symbol = int; }}

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
)_");

    auto const& rules = shared_rules.get();
    auto const& workflow = shared_workflow.get();

    for (auto [id, rule] : rules)
      format::print(
        std::cout,
        R"_(
#define FEEDBACK_{uppercase_id}_MATCH(match, highlighting) FEEDBACK_{response}_RESPONSE({id}, "{summary} [{type} from file://{feedback_rules}]\n |\n | " match "\n | " highlighting "\n |\n | RATIONALE : {rationale}\n | WORKAROUND: {workaround}\n |"))_",
"feedback_rules"_a = format::as_literal{ rules.origin() },
"id"_a = id,
"uppercase_id"_a = format::uppercase{ id },
"response"_a = format::uppercase{ to_string(workflow.at(rule.type).response) },
"type"_a = format::as_literal{ rule.type },
"summary"_a = format::as_literal{ rule.summary },
"rationale"_a = format::as_literal{ rule.rationale },
"workaround"_a = format::as_literal{ rule.workaround });
  }

  void print_matches(std::string const& source_files, std::function<void(std::string const&)> print_matches_from)
  {
    auto tasks = std::vector<async::task>{};

    auto content = std::ifstream{ source_files };
    for (std::string source_file; std::getline(content, source_file);)
      tasks.emplace_back([=] { print_matches_from(source_file); });
  }

  using changed_lines = container::interval_map<int, bool>;
  using changed_files = std::unordered_map<std::string, changed_lines>;

  auto parse_diff_block(std::string_view diff_block, changed_lines changes = changed_lines{ false }) -> changed_lines
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

    auto file_changes = std::move(changes.at(filename));

    auto search = text::forward_search{ diff_section };
    while (search.next(block_pattern))
    {
      auto const block = search.matched_text();
      file_changes = parse_diff_block(block, std::move(file_changes));
    }

    changes.at(filename) = std::move(file_changes);
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
      changes = parse_diff_section(section, std::move(changes));
    }

    return changes;
  }

  auto diff_from(std::string const& filename) -> changed_files
  {
    if (filename.empty())
      return {};

    return parse_diff(content_from(filename));
  }

  auto async_diff_from(std::string const& filename) -> std::shared_future<changed_files>
  {
    return async::share([=] { return diff_from(filename); });
  }

  bool ends_with(std::string const& str, std::string const& suffix) {
    if (str.length() < suffix.length())
      return false;

    return (0 == str.compare(str.length() - suffix.length(), suffix.length(), suffix));
  }

  auto make_filter_function(std::shared_future<feedback::workflow> shared_workflow, std::shared_future<changed_files> shared_changes)
  {
    return [shared_workflow, shared_changes](feedback::rule const& rule, std::string const& filename, int line_number)
    {
      auto const& workflow = shared_workflow.get();

      switch (workflow.at(rule.type).check)
      {
      case feedback::check::NO_CODE_LINES: [[fallthrough]];
      case feedback::check::NO_FILES: return false;
      case feedback::check::ALL_CODE_LINES: [[fallthrough]];
      case feedback::check::ALL_FILES: return true;
      case feedback::check::CHANGED_FILES:
      {
        for (auto const& [changed_filename, changed_code_lines] : shared_changes.get())
          if (ends_with(filename, changed_filename))
            return !changed_code_lines.is_constant();
      }
      break;
      case feedback::check::CHANGED_CODE_LINES:
      {
        for (auto const& [changed_filename, changed_code_lines] : shared_changes.get())
          if (ends_with(filename, changed_filename))
            return !changed_code_lines.is_constant();
      }
      break;
      }

      return false;
    };
  }

} // namespace

int main(int argc, char* argv[])
{
  std::ios::sync_with_stdio(false);

  try
  {
    auto const parameters = parameter::parse(argc, argv);

    auto const shared_rules = async_rules_from(parameters.rules_filename);
    auto const shared_workflow = async_workflow_from(parameters.workflow_filename);
    auto const shared_diff = async_diff_from(parameters.diff_filename);

    auto const redirected_output = stream::redirect(std::cout, parameters.output_filename);

    auto const filter_function = make_filter_function(shared_workflow, shared_diff);
    auto const print_matches_function = make_print_matches_function(shared_rules, filter_function);

    print_header(shared_rules, shared_workflow);
    print_matches(parameters.sources_filename, print_matches_function);
  }
  catch (std::exception const& e)
  {
    std::cerr << e.what() << '\n';
    return 1;
  }

  return 0;
}
