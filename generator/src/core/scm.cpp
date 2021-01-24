#include "feedback/scm.h"

#include "feedback/regex.h"
#include "feedback/text.h"

#include <charconv>

namespace feedback::scm {

  namespace {
    auto parse_starting_line(std::string_view block) {
      auto const starting_line_pattern = regex::compile("@@ [-][,0-9]+ [+]([0-9]+)[, ].*@@");

      regex::match starting_line;
      if (not starting_line_pattern.matches(block, { &starting_line }))
        return 0;

      int line_number = 0;
      std::from_chars(starting_line.data(), starting_line.data() + starting_line.size(), line_number);

      return line_number;
    }

    auto parse_filename(std::string_view section) -> std::string_view {
      auto filename_pattern = regex::compile("\n[-][-][-] a/.+\n[+][+][+] b/(.+)\n");

      regex::match filename;
      if (not filename_pattern.matches(section, { &filename }))
        return {};

      return filename;
    }
  } // namespace

  auto diff::changes::parse_from(std::string_view block, changes merged) -> changes {
    int line_number = parse_starting_line(block);
    if (not line_number)
      return merged;

    auto const line_pattern = regex::compile("\n([+ ])");

    auto search = text::forward_search{ block };
    while (search.next(line_pattern)) {
      auto const line = search.matched_text();

      if (line[0] == '+')
        merged.modified.assign(line_number, line_number + 1, true);

      ++line_number;
    }

    return merged;
  }

  auto diff::changes_from(std::string_view filename) const -> changes {
    for (auto const& [changed_filename, changed_lines] : modifications)
      if (text::ends_with(filename, changed_filename))
        return changed_lines;

    return {};
  }

  auto diff::parse(std::string_view output, diff merged) -> diff {
    // FIXME: debug me: auto const section_pattern = regex::compile("(?:^|\n)([a-z].*\n)+([-][-][-] a/.+\n[+][+][+] b/(.+)\n([-+ @].*\n)*)");
    auto const section_pattern =
    regex::compile("(?:^|\n)((?:[a-z].*\n)+[-][-][-] a/.+\n[+][+][+] b/(.+)\n([-+ @].*\n)*)");

    auto search = text::forward_search{ output };
    while (search.next(section_pattern))
      merged.parse_section(search.matched_text());

    return merged;
  }

  void diff::parse_section(std::string_view section) {
    auto const filename = std::string{ parse_filename(section) };
    if (filename.empty())
      return;

    auto& modified = modifications[filename];

    auto block_pattern = regex::compile("\n(@@ [-][,0-9]+ [+][,0-9]+ @@.*\n([-+ ].*\n)*)");

    auto search = text::forward_search{ section };
    while (search.next(block_pattern))
      modified = changes::parse_from(search.matched_text(), std::move(modified));
  }
} // namespace feedback::scm
