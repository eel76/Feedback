#pragma once
#include "generator/container.h"

#include <filesystem>
#include <string_view>
#include <unordered_map>

namespace generator::scm {
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

      static auto parse_from(std::string_view block, changes merged) -> changes;
      static auto parse_from(std::string_view block) -> changes {
        return parse_from(block, {});
      }

    private:
      container::interval_map<int, bool> modified{ false };
    };

    static auto parse(std::string_view output, diff merged = {}) -> diff;
    auto        changes_from(std::filesystem::path const& source) const -> changes;

  private:
    void parse_section(std::string_view section);

    struct hash {
      std::size_t operator()(std::filesystem::path const& source) const noexcept {
        return hash_value(source);
      }
    };

    std::unordered_map<std::filesystem::path, changes, hash> modifications;
  };
} // namespace generator::scm
