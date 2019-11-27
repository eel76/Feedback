#include "async.h"
#include "benchmark.h"
#include "format.h"
#include "parameter.h"
#include "stream.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <map>
#include <optional>
#include <regex>
#include <syncstream>
#include <unordered_set>

namespace {

auto make_string_view(std::ssub_match const& match)
{
  return std::string_view{ &*match.first, static_cast<size_t>(match.length()) };
}

struct coding_guidelines
{
  struct identifier
  {
    explicit identifier(std::string const& str)
    : origin(str)
    , prefix(origin)
    , nr()
    {
      std::smatch match;
      if (!std::regex_match(origin, match, std::regex{ "^([^\\d]+)(\\d{1,5})$" }))
        return;

      prefix = make_string_view(match[1]);
      nr = std::stoi(match[2].str());
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
    std::regex matched_files;
    std::regex ignored_files;
    std::regex matched_text;
    std::regex ignored_text;
    std::regex marked_text;
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
  rule.matched_files = std::regex{ json.at("matched_files").get<std::string>(), std::regex::optimize };
  rule.ignored_files = std::regex{ json.at("ignored_files").get<std::string>(), std::regex::optimize };
  rule.matched_text = std::regex{ json.at("matched_text").get<std::string>(), std::regex::optimize };
  rule.ignored_text = std::regex{ json.at("ignored_text").get<std::string>(), std::regex::optimize };
  rule.marked_text = std::regex{ json.at("marked_text").get<std::string>(), std::regex::optimize };
}

auto coding_guidelines_read_from(std::string const& file_name)
{
  auto json = nlohmann::json{};
  stream::read_from(file_name) >> json;

  return coding_guidelines{ file_name, json.get<coding_guidelines::rule_map>() };
}

auto check_rule_in(std::string const& file_name)
{
  auto const file_content = async::share([=] { return stream::content(stream::read_from(file_name)); });

  return [=](coding_guidelines const& guidelines, auto const& id, auto const& rule) {

    using fmt::operator""_a;

    if (!std::regex_search(file_name, rule.matched_files))
      return fmt::format("\n// rule {} not matched\n", id);

    if (std::regex_search(file_name, rule.ignored_files))
      return fmt::format("\n// rule {} ignored\n", id);

    auto out = std::ostringstream{};
    format::print(out, "\n// rule {} checked\n", id);

    ptrdiff_t nr = 0;

    std::for_each(
      std::sregex_iterator{ begin(file_content.get()), end(file_content.get()), rule.matched_text },
      std::sregex_iterator{},
      [&](auto const& sub_match) {
        auto const prefix = make_string_view(sub_match.prefix());
        auto const suffix = make_string_view(sub_match.suffix());

        auto const last_newline_pos = prefix.find_last_of('\n');
        auto const first_newline_pos = suffix.find_first_of('\n');

        auto const match_prefix =
          (last_newline_pos == std::string_view::npos) ? prefix : prefix.substr(last_newline_pos + 1);

        auto const match_suffix =
          (first_newline_pos == std::string_view::npos) ? suffix : suffix.substr(0, first_newline_pos);

        auto const match = std::string_view{ &*(sub_match[0].first), static_cast<size_t>(sub_match.length()) };

        std::string indent;
        std::string marker;
        std::smatch marked;

        if (std::regex_search(sub_match[0].first, sub_match[0].second, marked, rule.marked_text))
        {
          indent.resize(match_prefix.length() + marked.prefix().length(), ' ');
          marker.resize(marked.length(), '~');
        }

        std::string source;
        source.append(match_prefix);
        source.append(match);
        source.append(match_suffix);

        auto const ignored = std::regex_search(source, rule.ignored_text) ? "// Ignored: " : "";

        // FIXME: marker, wenn match über mehrere zeilen geht

        nr += std::count(begin (prefix), end (prefix), '\n');

        format::print(
          out,
          R"message(
{ignored}#if defined(__GNUC__)
{ignored}# line {nr} "{file_name}"
{ignored}# pragma GCC warning \
{ignored}{indent}"{id}: {summary} [{origin}]\n SEVERITY  : {severity}\n RATIONALE : {rationale}\n WORKAROUND: {workaround}"
{ignored}#elif defined(_MSC_VER)
{ignored}# line {nr} "{file_name}"
{ignored}
{ignored}# pragma message(__FILE__ "(" STRINGIFY(__LINE__) "): warning: {id}: {summary} [{origin}]\n SEVERITY  : {severity}\n RATIONALE : {rationale}\n WORKAROUND: {workaround}\n {source}\n {indent}{marker}")
{ignored}#endif
)message",
          "ignored"_a = ignored,
          "indent"_a = indent,
          "marker"_a = marker,
          "nr"_a = nr,
          "file_name"_a = format::as_literal{ file_name },
          "source"_a = format::as_literal{ source },
          "id"_a = format::as_literal{ id },
          "origin"_a = format::as_literal{ guidelines.origin },
          "summary"_a = format::as_literal{ rule.summary },
          "severity"_a = format::as_literal{ rule.severity },
          "rationale"_a = format::as_literal{ rule.rationale },
          "workaround"_a = format::as_literal{ rule.workaround });

        nr += std::count(begin (match), end (match), '\n');
      });

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
auto check_file(Guidelines&& guidelines, Filter&& filter)
{
  return [guidelines = std::forward<Guidelines>(guidelines),
          filter = std::forward<Filter>(filter)](std::string const& file_name) {
    if (!std::invoke(filter, file_name))
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

  if (!files_to_check.empty())
  {
    file_names.emplace();
    stream::for_each_line(stream::read_from(files_to_check), [&file_names](auto const& file_name) { file_names->emplace(file_name); });
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
    stream::for_each_line(stream::read_from(parameters.source_files), async::as_task(check_file(guidelines, filter)));
  }
  catch (std::exception const& e)
  {
    std::cerr << e.what() << '\n';
    return 1;
  }

  return 0;
}
