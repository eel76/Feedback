#pragma once
#include <initializer_list>
#include <memory>
#include <string_view>

namespace feedback::regex {

  using match = std::string_view;

  class precompiled {
  public:
    precompiled() = default;

    auto matches(std::string_view input) const -> bool;
    auto matches(std::string_view input, std::initializer_list<match*> captures_ret) const -> bool;

    auto find(std::string_view input, match* match_ret, match* skipped_ret, match* remaining_ret) const -> bool;

  private:
    explicit precompiled(std::string_view pattern);
    friend auto compile(std::string_view pattern) -> precompiled;

  private:
    struct impl;
    std::shared_ptr<impl> engine;
  };

  auto compile(std::string_view pattern) -> precompiled;
  auto capture(std::string_view pattern) -> std::string;
} // namespace feedback::regex
