#pragma once
#include "generator/regex.h"

#include <string>
#include <unordered_map>
#include <variant>

namespace generator::feedback {
  struct none {};
  struct message {};
  struct warning {};
  struct error {};

  using response = std::variant<message, warning, error, none>;

  auto to_string(feedback::response response) -> std::string_view;

  auto parse_response(std::string_view text) -> response;

  enum class check { ALL_FILES, ALL_LINES, CHANGED_FILES, CHANGED_LINES, NO_FILES, NO_LINES };

  struct handling {
    feedback::check    check{ check::ALL_FILES };
    feedback::response response;
  };

  using handlings = std::unordered_map<std::string, handling>;

  class workflow {
  public:
    explicit workflow(feedback::handlings handlings = {});

    auto operator[](std::string const& type) const -> handling;

  private:
    handlings handlings_;
  };

  struct rule {
    std::string        type;
    std::string        summary;
    std::string        rationale;
    std::string        workaround;
    regex::precompiled matched_files;
    regex::precompiled ignored_files;
    regex::precompiled matched_text;
    regex::precompiled ignored_text;
    regex::precompiled marked_text;
  };

  using rules = std::unordered_map<std::string, rule>;
} // namespace generator::feedback
