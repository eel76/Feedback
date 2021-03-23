#pragma once
#include "generator/regex.h"

#include <string>
#include <unordered_map>
#include <variant>

namespace generator::feedback {

  struct everything {};
  struct changed_files {};
  struct changed_lines {};
  struct nothing {};

  using check = std::variant<everything, changed_files, changed_lines, nothing>;

  auto to_string(feedback::check check) -> std::string_view;

  struct none {};
  struct message {};
  struct warning {};
  struct error {};

  using response = std::variant<message, warning, error, none>;

  auto to_string(feedback::response response) -> std::string_view;

  struct handling {
    feedback::check    check;
    feedback::response response;
  };

  using handlings = std::unordered_map<std::string, handling>;

  class workflow {
  public:
    explicit workflow(feedback::handlings handlings = {}) noexcept;

    auto operator[](std::string const& type) const noexcept -> handling;

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
