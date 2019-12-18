#include "async.h"
#include "benchmark.h"
#include "format.h"
#include "parameter.h"
#include "regex.h"
#include "stream.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <map>
#include <optional>
#include <syncstream>
#include <unordered_set>

namespace {

struct coding_guidelines
{
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

  using rule_map = std::map<identifier, rule>;

  std::string origin;
  rule_map rules;
};

void from_json(nlohmann::json const& json, coding_guidelines::rule& rule)
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

auto coding_guidelines_read_from(std::string const& file_name)
{
  auto json = nlohmann::json{};
  stream::from(file_name) >> json;

  return coding_guidelines{ file_name, json.get<coding_guidelines::rule_map>() };
}

auto first_line_of(std::string_view text)
{
  auto const end_of_first_line = text.find_first_of('\n');
  if (end_of_first_line == std::string_view::npos)
    return text;

  return text.substr(0, end_of_first_line);
}

auto last_line_of(std::string_view text)
{
  auto const end_of_penultimate_line = text.find_last_of('\n');
  if (end_of_penultimate_line == std::string_view::npos)
    return text;

  return text.substr(end_of_penultimate_line + 1);
}

auto matched_lines()
{

}

auto check_rule_in(std::string const& file_name)
{
  auto const file_content = async::share([=] { return stream::content(stream::from(file_name)); });

  return [=](coding_guidelines const& guidelines, auto const& id, auto const& rule) {
    using fmt::operator""_a;

    if (not rule.matched_files.matches(file_name))
      return fmt::format("\n// rule {} not matched\n", id);

    if (rule.ignored_files.matches(file_name))
      return fmt::format("\n// rule {} ignored\n", id);

    auto out = std::ostringstream{};
    format::print(out, "\n// rule {} checked\n", id);

    ptrdiff_t nr = 0;

    auto remaining = std::string_view{ file_content.get() };
    auto processed = std::string_view{ remaining.data(), 0 };
    auto skipped = std::string_view{};
    auto match = std::string_view{};

    while (rule.matched_text.find(remaining, &match, &skipped, &remaining))
    {
      if (match.empty())
        break;

      nr += std::count(begin(skipped), end(skipped), '\n');
      processed = std::string_view{ processed.data(), processed.length() + skipped.length() };

      // what if match starts/ends with a newline ???

      std::string matched_lines;
      std::string indentation;
      std::string annotation;

      matched_lines.append(last_line_of(processed));
      indentation.append(matched_lines.length(), ' ');
      matched_lines.append(match);
      matched_lines.append(first_line_of(remaining));

      if (auto marked = std::string_view{}; rule.marked_text.find(match, &marked, &skipped, nullptr))
      {
        indentation.append(skipped.length(), ' ');
        annotation.append(marked.length(), '~');

        if (not annotation.empty())
          annotation[0] = '^';
      }

      auto const first_matched_line = first_line_of(matched_lines);

      assert(first_matched_line.length() >= indentation.size());
      annotation.resize(first_matched_line.length() - indentation.size(), ' ');

      auto const ignored = rule.ignored_text.matches(matched_lines) ? "// Ignored: " : "";

      format::print(
        out,
        R"message(
{ignored}#if defined(__GNUC__)
{ignored}# line {nr} "{file_name}"
{ignored}# pragma GCC warning \
{ignored}{indentation}"{id}: {summary} [{origin}]\n SEVERITY  : {severity}\n RATIONALE : {rationale}\n WORKAROUND: {workaround}"
{ignored}#elif defined(_MSC_VER)
{ignored}# line {nr} "{file_name}"
{ignored}
{ignored}# pragma message(__FILE__ "(" STRINGIFY(__LINE__) "): warning: {id}: {summary} [{origin}]\n SEVERITY  : {severity}\n RATIONALE : {rationale}\n WORKAROUND: {workaround}\n {first_matched_line}\n {indentation}{annotation}")
{ignored}#endif
)message",
        "ignored"_a = ignored,
        "indentation"_a = indentation,
        "annotation"_a = annotation,
        "nr"_a = nr,
        "file_name"_a = format::as_literal{ file_name },
        "first_matched_line"_a = format::as_literal{ first_matched_line },
        "id"_a = format::as_literal{ id },
        "origin"_a = format::as_literal{ guidelines.origin },
        "summary"_a = format::as_literal{ rule.summary },
        "severity"_a = format::as_literal{ rule.severity },
        "rationale"_a = format::as_literal{ rule.rationale },
        "workaround"_a = format::as_literal{ rule.workaround });

      nr += std::count(begin(match), end(match), '\n');
    }

    return out.str();
  };
}

auto async_messages_from(coding_guidelines const& guidelines, std::string const& file_name)
{
  auto const check_rule_in_file = check_rule_in(file_name);

  auto messages = std::vector<std::future<std::string>>{};

  for (auto const& [id, rule] : guidelines.rules)
    messages.push_back(async::launch(check_rule_in_file, guidelines, id, rule));

  return messages;
}

template <class Guidelines, class Filter>
auto print_messages(Guidelines&& guidelines, Filter&& filter)
{
  return [guidelines = std::forward<Guidelines>(guidelines),
          filter = std::forward<Filter>(filter)](std::string const& file_name) {
    if (not std::invoke(filter, file_name))
      return;

    auto messages = async_messages_from(guidelines.get(), file_name);

    auto synchronized_out = std::osyncstream{ std::cout };
    format::print(synchronized_out, "\n// {}\n", file_name);

    for (auto&& message : messages)
      synchronized_out << message.get();
  };
}

auto filter_read_from(std::string const& files_to_check)
{
  auto file_names = std::optional<std::unordered_set<std::string>>{};

  if (not files_to_check.empty())
  {
    file_names.emplace();
    stream::for_each_line(stream::from(files_to_check), [&](auto const& file_name) { file_names->emplace(file_name); });
  }

  return [file_names = std::move(file_names)](auto const& file_name) {
    return !file_names || file_names->count(file_name) != 0;
  };
}

void print_header()
{
  std::cout << R"header(// DO NOT EDIT: this file is generated automatically

namespace { using symbol = int; }

#define __STRINGIFY(x) #x
#define STRINGIFY(x) __STRINGIFY(x)
)header";
}

auto redirect_cout_to(std::string const& file_name)
{
  struct redirection
  {
    redirection(std::string const& file_name)
    : stream(file_name)
    , backup(std::cout.rdbuf(stream.rdbuf()))
    {
    }
    ~redirection()
    {
      std::cout.rdbuf(backup);
    }

  private:
    std::ofstream stream;
    std::streambuf* backup;
  };

  if (file_name.empty())
    return std::unique_ptr<redirection>{};

  return std::make_unique<redirection>(file_name);
}

} // namespace

int main(int argc, char* argv[])
{
  auto const overall_duration = benchmark::milliseconds_scope{ "Overall duration in milliseconds: {}\n" };

  std::ios::sync_with_stdio(false);

  try
  {
    auto const parameters = parameter::parse(argc, argv);

    auto const guidelines = async::share([=] { return coding_guidelines_read_from(parameters.coding_guidelines); });

    // FIXME: async filter
    auto const filter = filter_read_from(parameters.files_to_check);

    auto const output = redirect_cout_to(parameters.output_file);

    print_header();

    stream::for_each_line(stream::from(parameters.source_files), async::as_task(print_messages(guidelines, filter)));
  }
  catch (std::exception const& e)
  {
    std::cerr << e.what() << '\n';
    return 1;
  }

  return 0;
}
