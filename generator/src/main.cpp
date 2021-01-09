#include "feedback/async.h"
#include "feedback/format.h"
#include "feedback/io.h"
#include "feedback/parameter.h"
#include "feedback/regex.h"
#include "feedback/text.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cxx20/syncstream>
#include <map>
#include <optional>
#include <type_traits>
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

  class workflow : public std::map<std::string, handling>
  {
  public:
    workflow()
    {
      insert_or_assign("requirement", handling{ check::ALL_FILES, response::ERROR });
      insert_or_assign("guideline", handling{ check::ALL_FILES, response::WARNING });
      insert_or_assign("improvement", handling{ check::CHANGED_FILES, response::WARNING });
      insert_or_assign("suggestion", handling{ check::CHANGED_CODE_LINES, response::WARNING });
    }

  public:
    static auto parse_from(std::istream& input_stream)
    {
      auto json = nlohmann::json{};
      input_stream >> json;

      return json.get<workflow>();
    }
  };

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
    rule.type = "requirement"; // FIXME: json.at("type");
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

  class rules : public std::map<identifier, rule>
  {
  public:
    static auto parse_from(std::istream& input_stream)
    {
      auto json = nlohmann::json{};
      input_stream >> json;

      return json.get<rules>();
    }
  };

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

  auto workflow_from(std::string const& text)
  {
    return nlohmann::json::parse(text).get<feedback::workflow>();;
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
    using fmt::operator""_a;

    return[filename,
      file_content = async_content_from(filename)](auto const& id, auto const& rule, std::string const& origin) {

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
          format::print(out, "\n// ignored match in line {nr}", "nr"_a = search.line());
          continue;
        }

        auto const marked_text = search_marked_text(search.matched_text(), rule.marked_text);
        auto const highlighting = excerpt{ matched_lines, marked_text };

        format::print(
          out,
          R"_(
# line {nr}
{indentation}FEEDBACK_{uppercase_type}({id}, "{summary} [{type} from file://{origin}]\n |\n | {first_matched_line}\n | {indentation}{annotation}\n |\n | RATIONALE : {rationale}\n | WORKAROUND: {workaround}\n |")
)_",
"indentation"_a = highlighting.indentation,
"annotation"_a = highlighting.annotation,
"nr"_a = search.line() + 1,
"first_matched_line"_a = format::as_literal{ highlighting.first_line },
"id"_a = id,
"origin"_a = format::as_literal{ origin },
"uppercase_type"_a = format::uppercase{ rule.type },
"type"_a = format::as_literal{ rule.type },
"summary"_a = format::as_literal{ rule.summary },
"rationale"_a = format::as_literal{ rule.rationale },
"workaround"_a = format::as_literal{ rule.workaround });
      }

      return out.str();
    };
  }

  auto async_messages_from(std::string const& rules_file, feedback::rules const& rules, std::string const& source_file)
  {
    auto const check_rule_in_file_function = make_check_rule_in_file_function(source_file);

    auto messages = std::vector<std::future<std::string>>{};

    for (auto const& [id, rule] : rules)
      messages.push_back(async::launch(check_rule_in_file_function, id, rule, rules_file));

    return messages;
  }

  using filter = std::function<bool(std::string const&)>;

  auto parse_filter_from(std::istream& input_stream)
  {
    auto filenames = std::unordered_set<std::string>{};

    for (std::string filename; std::getline(input_stream, filename);)
      filenames.emplace(filename);

    return filter{ [filenames = std::move(filenames)] (auto const& filename) {
      return filenames.count(filename) != 0;
    } };
  }

  auto read_rules_from(std::string const& filename)
  {
    return async::launch([filename] {
      auto input_stream = std::ifstream{ filename };
      return feedback::rules::parse_from(input_stream);
      })
      .share();
  }

  auto read_files_to_check_from(std::string const& filename)
  {
    return async::launch([=] {
      if (filename.empty())
        return filter{ [](auto const&) { return true; } };

      auto input_stream = std::ifstream{ filename };
      return parse_filter_from(input_stream);
      })
      .share();
  }

  auto make_check_rules_function(std::ostream& output, std::string const& rules_file, std::string const& files_to_check)
  {
    return[&output,
      rules_file,
      rules = read_rules_from(rules_file),
      check_filename = read_files_to_check_from(files_to_check)](std::string const& filename) {
      if (not std::invoke(check_filename.get(), filename))
        return;

      auto messages = async_messages_from(rules_file, rules.get(), filename);

      auto synchronized_out = cxx20::osyncstream{ output };
      format::print(synchronized_out, "\n\n# line 1 \"{}\"", filename);

      for (auto&& message : messages)
        synchronized_out << message.get();
    };
  }

  void print_header(std::ostream& output, feedback::workflow workflow)
  {
    format::print(
      output,
      R"_(// DO NOT EDIT: this file is generated automatically

namespace {{ using avoid_compiler_warnings_symbol = int; }}

#define __STRINGIFY(x) #x
#define STRINGIFY(x) __STRINGIFY(x)
#define PRAGMA(x) _Pragma (#x)

#if defined (__GNUC__)
# define FEEDBACK_ERROR(id, msg) PRAGMA(GCC error STRINGIFY(id) ": " msg)
# define FEEDBACK_WARNING(id, msg) PRAGMA(GCC warning STRINGIFY(id) ": " msg)
#else
# define FEEDBACK_ERROR(id, msg) PRAGMA(message (__FILE__ "(" STRINGIFY(__LINE__) "): error " STRINGIFY(id) ": " msg))
# define FEEDBACK_WARNING(id, msg) PRAGMA(message (__FILE__ "(" STRINGIFY(__LINE__) "): warning " STRINGIFY(id) ": " msg))
#endif
)_");

    for (auto const& [type, handling] : workflow)
      format::print(
        output,
        R"_(
#define FEEDBACK_{}(id, msg) FEEDBACK_{}(id, msg))_", format::uppercase{ type }, format::uppercase{ to_string(handling.response) });
  }

  void check_source_files(std::string const& source_files, std::function<void(std::string const&)> print_messages_from)
  {
    auto tasks = std::vector<async::task>{};

    auto content = std::ifstream{ source_files };
    for (std::string source_file; std::getline(content, source_file);)
      tasks.emplace_back([=] { print_messages_from(source_file); });
  }

  auto read_workflow_from(std::string const& filename) -> feedback::workflow
  {
    if (filename.empty())
      return {};

    auto ifstream = std::ifstream{ filename };
    return nlohmann::json::parse(io::content (ifstream)).get<feedback::workflow>();
  }

} // namespace

int main(int argc, char* argv[])
{
  std::ios::sync_with_stdio(false);

  try
  {
    auto const parameters = parameter::parse(argc, argv);
    auto const redirected_output = io::redirect(std::cout, parameters.output_filename);
    auto const check_rules_function =
      make_check_rules_function(std::cout, parameters.rules_filename, parameters.files_to_check);

    print_header(std::cout, read_workflow_from (parameters.workflow_filename));
    check_source_files(parameters.sources_filename, check_rules_function);
  }
  catch (std::exception const& e)
  {
    std::cerr << e.what() << '\n';
    return 1;
  }

  return 0;
}
