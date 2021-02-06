#pragma once
#include "generator/regex.h"

#include <string>
#include <unordered_map>

namespace generator::feedback {
  enum class response { NONE, MESSAGE, WARNING, ERROR };

  enum class check { ALL_FILES, ALL_LINES, CHANGED_FILES, CHANGED_LINES, NO_FILES, NO_LINES };

  struct handling {
    feedback::check    check{ check::ALL_FILES };
    feedback::response response{ response::MESSAGE };
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
