#include "generator/text.h"

#include "generator/regex.h"

#include <algorithm>
#include <cassert>

namespace generator::text {

  namespace {
    auto operator|(std::string_view const& lhs, std::string_view const& rhs) {
      assert(lhs.data() + lhs.length() == rhs.data());
      return std::string_view{ lhs.data(), lhs.length() + rhs.length() };
    }
  } // namespace

  excerpt::excerpt(std::string_view text, std::string_view match) {
    assert(text.data() <= match.data());
    assert(text.data() + text.length() >= match.data() + match.length());

    first_line = text::first_line_of(text);

    indentation.append(match.data() - text.data(), ' ');
    annotation.append(text::first_line_of(match).length(), '~');

    if (not annotation.empty())
      annotation[0] = '^';
  }

  auto forward_search::highlighted_text(regex::precompiled const& pattern) const -> excerpt {
    if (auto highlighting = forward_search{ matched_text() }; highlighting.next(pattern))
      return { matched_lines(), highlighting.matched_text() };

    return { matched_lines(), matched_text() };
  }

  auto forward_search::next(regex::precompiled const& pattern) -> bool {
    skip(current_match);

    std::string_view no_match;

    if (!pattern.find(remaining, &current_match, &no_match, &remaining))
      return false;

    skip(no_match);

    last_processed_line  = text::last_line_of(processed);
    first_remaining_line = text::first_line_of(remaining);

    return !current_match.empty();
  }

  auto forward_search::next_but(regex::precompiled const& pattern, regex::precompiled const& ignored_pattern) -> bool {
    while (next(pattern))
      if (not ignored_pattern.matches(matched_text()))
        return true;

    return false;
  }

  std::string_view forward_search::matched_lines() const {
    return last_processed_line | current_match | first_remaining_line;
  }

  void forward_search::skip(std::string_view const& text) {
    processed = processed | text;
    processed_line_count += std::count(begin(text), end(text), '\n');
  }
} // namespace generator::text
