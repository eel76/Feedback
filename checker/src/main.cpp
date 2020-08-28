#include "feedback/async.h"
#include "feedback/format.h"
#include "feedback/io.h"
#include "feedback/parameter.h"
#include "feedback/regex.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cxx20/syncstream>
#include <map>
#include <optional>
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

auto read_coding_guidelines_from(std::string const& file_name)
{
  auto json = nlohmann::json{};
  io::stream_from(file_name) >> json;

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

template <class Range, class Element>
auto count(Range const& range, Element&& element)
{
  return std::count(begin(range), end(range), std::forward<Element>(element));
}

auto operator|(std::string_view const& lhs, std::string_view const& rhs)
{
  assert(lhs.data() + lhs.length() == rhs.data());
  return std::string_view{ lhs.data(), lhs.length() + rhs.length() };
}

auto check_rule_in_file_function(std::string const& file_name)
{
  auto const file_content = async::launch([=] { return io::content(io::stream_from(file_name)); }).share();

  return [=](coding_guidelines const& guidelines, auto const& id, auto const& rule) {
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
         nr += count(match, '\n'))
    {
      if (match.empty())
        break;

      nr += count(skipped, '\n');
      processed = processed | skipped;

      // continue if line filtered out

      auto const last_processed_line = last_line_of(processed);
      auto const first_remaining_line = first_line_of(remaining);

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

      auto const first_matched_line = first_line_of(matched_lines);
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
        "origin"_a = format::as_literal{ guidelines.origin },
        "summary"_a = format::as_literal{ rule.summary },
        "severity"_a = format::as_literal{ rule.severity },
        "rationale"_a = format::as_literal{ rule.rationale },
        "workaround"_a = format::as_literal{ rule.workaround });
    }

    return out.str();
  };
}

auto async_messages_from(coding_guidelines const& guidelines, std::string const& file_name)
{
  auto const check_rule_in_file = check_rule_in_file_function(file_name);

  auto messages = std::vector<std::future<std::string>>{};

  for (auto const& [id, rule] : guidelines.rules)
    messages.push_back(async::launch(check_rule_in_file, guidelines, id, rule));

  return messages;
}

template <class Guidelines, class Filter>
auto print_messages_function(Guidelines&& guidelines, Filter&& filter)
{
  return [guidelines = std::forward<Guidelines>(guidelines),
          filter = std::forward<Filter>(filter)](std::string const& file_name) {
    if (not std::invoke(filter, file_name))
      return;

    auto messages = async_messages_from(guidelines.get(), file_name);

    auto synchronized_out = cxx20::osyncstream{ std::cout };
    format::print(synchronized_out, "\n// {}\n", file_name);

    for (auto&& message : messages)
      synchronized_out << message.get();
  };
}

auto read_filter_from(std::string const& files_to_check)
{
  auto file_names = std::optional<std::unordered_set<std::string>>{};

  if (not files_to_check.empty())
  {
    file_names.emplace();
    for_each_line(io::stream_from(files_to_check), [&](auto const& file_name) { file_names->emplace(file_name); });
  }

  return [file_names = std::move(file_names)](auto const& file_name) {
    return !file_names || file_names->count(file_name) != 0;
  };
}

void print_header()
{
  std::cout << R"_(// DO NOT EDIT: this file is generated automatically

namespace { using symbol = int; }

#define __STRINGIFY(x) #x
#define STRINGIFY(x) __STRINGIFY(x)
)_";
}

} // namespace

int main(int argc, char* argv[])
{
  std::ios::sync_with_stdio(false);

  try
  {
    auto const parameters = parameter::parse(argc, argv);

    auto const shared_guidelines =
      async::share([=] { return read_coding_guidelines_from(parameters.coding_guidelines); });
    auto const shared_filter = read_filter_from(parameters.files_to_check);

    auto const redirected_output = io::redirect(std::cout, parameters.output_file);

    print_header();

    for_each_line(
      io::stream_from(parameters.source_files), async::as_task(print_messages_function(shared_guidelines, shared_filter)));
  }
  catch (std::exception const& e)
  {
    std::cerr << e.what() << '\n';
    return 1;
  }

  return 0;
}
