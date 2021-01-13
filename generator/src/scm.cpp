#include "feedback/scm.h"

#include "feedback/regex.h"
#include "feedback/text.h"

#include <charconv>

namespace scm {

  auto diff::changes_from(std::string_view filename) const -> changes {
    for (auto const& [changed_filename, changed_code_lines] : modifications)
      if (text::ends_with(filename, changed_filename))
        return changed_code_lines;

    return {};
  }

  namespace {
    auto parse_diff_block(std::string_view block, diff::changes file_changes) -> diff::changes {
      auto starting_line_pattern = regex::compile("@@ [-][,0-9]+ [+]([0-9]+)[, ].*@@");
      auto line_pattern          = regex::compile("\n([+ ])");

      regex::match starting_line;
      if (not starting_line_pattern.matches(block, { &starting_line }))
        return file_changes;

      int line_number = 1;

      auto [p, ec] = std::from_chars(starting_line.data(), starting_line.data() + starting_line.size(), line_number);
      if (ec == std::errc::invalid_argument)
        return file_changes;

      auto line_search = text::forward_search{ block };
      while (line_search.next(line_pattern)) {
        auto const line = line_search.matched_text();

        if (line[0] == '+')
          file_changes.add(line_number);

        ++line_number;
      }

      return file_changes;
    }
  }

  auto diff::parse_from(std::string_view output, diff merged_diff) -> diff {
    // FIXME: debug me: auto const section_pattern = regex::compile("(?:^|\n)([a-z].*\n)+([-][-][-] a/.+\n[+][+][+] b/(.+)\n([-+ @].*\n)*)");
    auto const section_pattern =
    regex::compile("(?:^|\n)((?:[a-z].*\n)+[-][-][-] a/.+\n[+][+][+] b/(.+)\n([-+ @].*\n)*)");

    auto search = text::forward_search{ output };
    while (search.next(section_pattern)) {
      auto const section = search.matched_text();
      merged_diff.parse_section(section);
    }

    return merged_diff;
  }

  void diff::parse_section(std::string_view diff_section) {
    auto filename_pattern = regex::compile("\n[-][-][-] a/.+\n[+][+][+] b/(.+)\n");
    auto block_pattern    = regex::compile("\n(@@ [-][,0-9]+ [+][,0-9]+ @@.*\n([-+ ].*\n)*)");

    regex::match filename;
    if (not filename_pattern.matches(diff_section, { &filename }))
      return;

    auto& file_changes = modifications[std::string{ filename }];

    auto search = text::forward_search{ diff_section };
    while (search.next(block_pattern)) {
      auto const block = search.matched_text();
      file_changes     = parse_diff_block(block, std::move(file_changes));
    }

    return;
  }

} // namespace text
