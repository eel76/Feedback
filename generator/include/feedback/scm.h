#pragma once
#include "feedback/container.h"

#include <string>
#include <string_view>
#include <unordered_map>

namespace scm {
  class diff {
  public:
    class changes {
    public:
      auto empty() const {
        if (modified.is_constant())
          return not modified[1];

        return false;
      }

      auto operator[](int line) const {
        return modified[line];
      }

      static auto parse_from(std::string_view block, changes merged = {}) -> changes;

    private:
      container::interval_map<int, bool> modified{ false };
    };

    static auto parse_from(std::string_view output, diff merged = {}) -> diff;
    auto        changes_from(std::string_view filename) const -> changes;

  private:
    void parse_section(std::string_view section);

    std::unordered_map<std::string, changes> modifications;
  };
} // namespace scm
