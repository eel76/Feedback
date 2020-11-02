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
#include <unordered_set>

namespace feedback {
struct identifier
{
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

struct rule
{
  std::string severity;
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
  rule.severity = json.at("severity");
  rule.summary = json.at("summary");
  rule.rationale = json.at("rationale");
  rule.workaround = json.at("workaround");
  rule.matched_files = regex::compile(regex::capture(json.at("matched_files").get<std::string>()));
  rule.ignored_files = regex::compile(regex::capture(json.at("ignored_files").get<std::string>()));
  rule.matched_text = regex::compile(regex::capture(json.at("matched_text").get<std::string>()));
  rule.ignored_text = regex::compile(regex::capture(json.at("ignored_text").get<std::string>()));
  rule.marked_text = regex::compile(regex::capture(json.at("marked_text").get<std::string>()));
}

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

auto operator|(std::string_view const& lhs, std::string_view const& rhs)
{
  assert(lhs.data() + lhs.length() == rhs.data());
  return std::string_view{ lhs.data(), lhs.length() + rhs.length() };
}

auto read_content_from(std::string const& file_name)
{
  return async::launch([=] {
           auto input_stream = std::ifstream{ file_name };
           return io::content(input_stream);
         })
    .share();
}

auto make_check_rule_in_file_function(std::string const& file_name)
{
  return [file_name,
          file_content = read_content_from(file_name)](auto const& id, auto const& rule, std::string const& origin) {
    using fmt::operator""_a;

    if (not rule.matched_files.matches(file_name))
      return fmt::format("\n// rule {} not matched\n", id);

    if (rule.ignored_files.matches(file_name))
      return fmt::format("\n// rule {} ignored\n", id);

    auto out = std::ostringstream{};
    format::print(out, "\n// rule {} checked\n", id);

    auto remaining = std::string_view{ file_content.get() };
    auto processed = std::string_view{ remaining.data(), 0 };
    auto skipped = std::string_view{};
    auto match = std::string_view{};

    for (auto nr = ptrdiff_t{ 0 }; rule.matched_text.find(remaining, &match, &skipped, &remaining);
         nr += std::count(begin(match), end(match), '\n'))
    {
      if (match.empty())
        break;

      nr += std::count(begin(skipped), end(skipped), '\n');
      processed = processed | skipped;

      // continue if line filtered out

      auto const last_processed_line = text::last_line_of(processed);
      auto const first_remaining_line = text::first_line_of(remaining);

      std::string indentation;
      std::string annotation;

      indentation.append(last_processed_line.length(), ' ');

      if (auto marked = std::string_view{}; rule.marked_text.find(match, &marked, &skipped, nullptr))
      {
        indentation.append(skipped.length(), ' ');
        annotation.append(marked.length(), '~');

        if (not annotation.empty())
          annotation[0] = '^';
      }

      auto const matched_lines = last_processed_line | match | first_remaining_line;
      auto const ignored = rule.ignored_text.matches(matched_lines) ? "// Ignored: " : "";

      auto const first_matched_line = text::first_line_of(matched_lines);
      assert(first_matched_line.length() >= indentation.size());

      annotation.resize(first_matched_line.length() - indentation.size(), ' ');

      format::print(
        out,
        R"_(
{ignored}#if defined(__GNUC__)
{ignored}# line {nr} "{file_name}"
{ignored}# pragma GCC warning \
{ignored}{indentation}"{id}: {summary} [{origin}]\n SEVERITY  : {severity}\n RATIONALE : {rationale}\n WORKAROUND: {workaround}"
{ignored}#elif defined(_MSC_VER)
{ignored}# line {nr} "{file_name}"
{ignored}
{ignored}# pragma message(__FILE__ "(" STRINGIFY(__LINE__) "): warning: {id}: {summary} [{origin}]\n SEVERITY  : {severity}\n RATIONALE : {rationale}\n WORKAROUND: {workaround}\n {first_matched_line}\n {indentation}{annotation}")
{ignored}#endif
)_",
        "ignored"_a = ignored,
        "indentation"_a = indentation,
        "annotation"_a = annotation,
        "nr"_a = nr,
        "file_name"_a = format::as_literal{ file_name },
        "first_matched_line"_a = format::as_literal{ first_matched_line },
        "id"_a = format::as_literal{ id },
        "origin"_a = format::as_literal{ origin },
        "summary"_a = format::as_literal{ rule.summary },
        "severity"_a = format::as_literal{ rule.severity },
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
  auto file_names = std::unordered_set<std::string>{};

  for (std::string file_name; std::getline(input_stream, file_name);)
    file_names.emplace(file_name);

  return filter{ [file_names = std::move(file_names)](auto const& file_name) {
    return file_names.count(file_name) != 0;
  } };
}

auto read_rules_from(std::string const& file_name)
{
  return async::launch([file_name] {
           auto input_stream = std::ifstream{ file_name };
           return feedback::rules::parse_from(input_stream);
         })
    .share();
}

auto read_files_to_check_from(std::string const& file_name)
{
  return async::launch([=] {
           if (file_name.empty())
             return filter{ [](auto const&) { return true; } };

           auto input_stream = std::ifstream{ file_name };
           return parse_filter_from(input_stream);
         })
    .share();
}

auto make_check_rules_function(std::ostream& output, std::string const& rules_file, std::string const& files_to_check)
{
  return [&output,
          rules_file,
          rules = read_rules_from(rules_file),
          check_file_name = read_files_to_check_from(files_to_check)](std::string const& file_name) {
    if (not std::invoke(check_file_name.get(), file_name))
      return;

    auto messages = async_messages_from(rules_file, rules.get(), file_name);

    auto synchronized_out = cxx20::osyncstream{ output };
    format::print(synchronized_out, "\n// {}\n", file_name);

    for (auto&& message : messages)
      synchronized_out << message.get();
  };
}

void print_header(std::ostream& output)
{
  output << R"_(// DO NOT EDIT: this file is generated automatically

namespace { using symbol = int; }

#define __STRINGIFY(x) #x
#define STRINGIFY(x) __STRINGIFY(x)
)_";
}

void check_source_files(std::string const& source_files, std::function<void(std::string const&)> print_messages_from)
{
  auto tasks = std::vector<async::task>{};

  auto content = std::ifstream{ source_files };
  for (std::string source_file; std::getline(content, source_file);)
    tasks.emplace_back([=] { print_messages_from(source_file); });
}

} // namespace

int main(int argc, char* argv[])
{
  std::ios::sync_with_stdio(false);

  try
  {
    auto const parameters = parameter::parse(argc, argv);
    auto const redirected_output = io::redirect(std::cout, parameters.output_file);
    auto const check_rules_function =
      make_check_rules_function(std::cout, parameters.rules_file, parameters.files_to_check);

    print_header(std::cout);
    check_source_files(parameters.source_list_file, check_rules_function);
  }
  catch (std::exception const& e)
  {
    std::cerr << e.what() << '\n';
    return 1;
  }

  return 0;
}
