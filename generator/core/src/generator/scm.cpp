#include "generator/scm.h"

#include "generator/regex.h"
#include "generator/text.h"

#include <charconv>

namespace generator::scm {

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

    auto parse_filename(std::string_view section) -> std::filesystem::path {
      auto filename_pattern = regex::compile("\n[-][-][-] a/.+\n[+][+][+] b/(.+)\n");

      regex::match filename;
      if (not filename_pattern.matches(section, { &filename }))
        return {};

      return { filename };
    }

    auto ends_with(std::filesystem::path const& path, std::filesystem::path const& suffix) -> bool {
      auto const first1 = path.begin();
      auto const first2 = suffix.begin();

      auto itr1 = path.end();
      auto itr2 = suffix.end();

      while (itr2 != first2)
        if (itr1 == first1 || *--itr1 != *--itr2)
          return false;

      return true;
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

  auto diff::changes_from(std::filesystem::path const& source) const -> changes {
    for (auto const& [path, changed_lines] : modifications)
      if (ends_with(source, path))
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
    auto const filename = parse_filename(section);
    if (filename.empty())
      return;

    auto& modified = modifications[filename];

    auto block_pattern = regex::compile("(@@ [-][,0-9]+ [+][,0-9]+ @@.*\n([-+ ].*\n)*)");

    auto search = text::forward_search{ section };
    while (search.next(block_pattern))
      modified = changes::parse_from(search.matched_text(), std::move(modified));
  }
} // namespace generator::scm
