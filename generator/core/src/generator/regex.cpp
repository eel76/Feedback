#include "generator/regex.h"

#include <re2/re2.h>

#include <array>
#include <cassert>
#include <stdexcept>
#include <string_view>

namespace generator::regex {
  namespace {
    auto as_string_piece(std::string_view sv) {
      return re2::StringPiece{ sv.data(), sv.length() };
    }

    auto as_string_view(re2::StringPiece const& match) {
      return std::string_view{ match.data(), match.length() };
    }
  } // namespace

  class precompiled::impl : public re2::RE2 {
  public:
    explicit impl(std::string_view pattern) : RE2(as_string_piece(pattern), default_options()) {
    }

  private:
    static auto default_options() -> RE2::Options {
      auto options = RE2::Options{};
      options.set_longest_match(false);
      options.set_log_errors(true);
      return options;
    }
  };

  precompiled::precompiled(std::string_view pattern) : engine(std::make_shared<impl>(pattern)) {
    if (not engine->ok())
      throw std::invalid_argument{ std::string{ "Invalid regex: " }.append(engine->error()) };
  }

  auto precompiled::matches(std::string_view input) const -> bool {
    return matches(input, {});
  }

  auto precompiled::matches(std::string_view input, std::initializer_list<match*> captures_ret) const -> bool {
    auto constexpr max_captures = 64;

    std::array<re2::StringPiece, max_captures> re2_captures;
    std::array<re2::RE2::Arg, max_captures>    re2_args;
    std::array<re2::RE2::Arg*, max_captures>   re2_args_p;

    if (captures_ret.size() > re2_captures.size())
      return false;

    for (size_t i = 0; i < captures_ret.size(); ++i)
      re2_args_p[i] = &(re2_args[i] = &re2_captures[i]);

    if (not RE2::PartialMatchN(as_string_piece(input), *engine, re2_args_p.data(), static_cast<int>(captures_ret.size())))
      return false;

    for (size_t i = 0; i < captures_ret.size(); ++i)
      if (auto capture_ret = *(captures_ret.begin() + i))
        *capture_ret = as_string_view(re2_captures[i]);

    return true;
  }

  auto precompiled::find(std::string_view input, match* match_ret, match* skipped_ret, match* remaining_ret) const -> bool {
    auto remaining = as_string_piece(input);
    auto match     = re2::StringPiece{};

    // FIXME: FindAndConsume or Consume or something completely different?
    if (not RE2::FindAndConsume(&remaining, *engine, &match))
      return false;

    input.remove_suffix(match.length() + remaining.length());

    if (match_ret)
      *match_ret = as_string_view(match);

    if (skipped_ret)
      *skipped_ret = input;

    if (remaining_ret)
      *remaining_ret = as_string_view(remaining);

    return true;
  }

  auto compile(std::string_view pattern) -> precompiled {
    return precompiled{ pattern };
  }

  auto capture(std::string_view pattern) -> precompiled {
    return compile(std::string{ "(" }.append(pattern).append(")"));
  }
} // namespace generator::regex
