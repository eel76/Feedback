#pragma once
#include <string>
#include <string_view>

namespace generator::regex {
  class precompiled;
}

namespace generator::text {

  inline auto first_line_of(std::string_view text) -> std::string_view {
    auto const end_of_first_line = text.find_first_of('\n');
    if (end_of_first_line == std::string_view::npos)
      return text;

    return text.substr(0, end_of_first_line);
  }

  inline auto last_line_of(std::string_view text) -> std::string_view {
    auto const end_of_penultimate_line = text.find_last_of('\n');
    if (end_of_penultimate_line == std::string_view::npos)
      return text;

    return text.substr(end_of_penultimate_line + 1);
  }

  inline auto ends_with(std::string_view text, std::string_view suffix) -> bool {
    if (text.length() < suffix.length())
      return false;

    return (0 == text.compare(text.length() - suffix.length(), suffix.length(), suffix));
  }

  class excerpt {
  public:
    excerpt(std::string_view text, std::string_view match);

  public:
    std::string_view first_line;
    std::string      indentation;
    std::string      annotation;
  };

  class forward_search {
  public:
    explicit forward_search(std::string_view const& text)
    : processed_line_count(0), processed(text.data(), 0), current_match(text.data(), 0), remaining(text) {
    }

    auto next(regex::precompiled const& pattern) -> bool;

    auto matched_text() const -> std::string_view {
      return current_match;
    }

    auto matched_lines() const -> std::string_view;

    auto line() const -> int {
      return static_cast<int>(processed_line_count);
    }

    auto column() const -> int {
      return static_cast<int>(last_processed_line.length());
    }

  private:
    void skip(std::string_view const& text);

  private:
    std::ptrdiff_t   processed_line_count;
    std::string_view processed;
    std::string_view last_processed_line;
    std::string_view current_match;
    std::string_view first_remaining_line;
    std::string_view remaining;
  };
} // namespace generator::text
