#pragma once

#include <fmt/format.h>

#include <iterator>
#include <string_view>

namespace format {

template <class Stream, class... Args>
void print(Stream&& out, std::string_view fmt, Args&&... args)
{
  fmt::format_to(std::ostream_iterator<char>(out), fmt, std::forward<Args>(args)...);
}

struct as_literal
{
  std::string_view str;
};

struct uppercase
{
  std::string_view str;
};

} // namespace format

namespace fmt {

template <>
struct formatter<format::as_literal>
{
  template <typename ParseContext>
  constexpr auto parse(ParseContext& ctx)
  {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(format::as_literal const& text, FormatContext& ctx)
  {
    auto out = ctx.out();

    for (auto const& ch : text.str)
    {
      switch (ch)
      {
      case '\n':
        out = base.format('\\', ctx);
        out = base.format('n', ctx);
        break;
      case '\r':
        break;
      case '\\':
        [[fallthrough]];
      case '\"':
        out = base.format('\\', ctx);
        [[fallthrough]];
      default:
        out = base.format(ch, ctx);
        break;
      }
    }

    return out;
  }

private:
  formatter<char> base;
};

template <>
struct formatter<format::uppercase>
{
  template <typename ParseContext>
  constexpr auto parse(ParseContext& ctx)
  {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(format::uppercase const& text, FormatContext& ctx)
  {
    auto out = ctx.out();

    for (auto const& ch : text.str)
      out = base.format(std::toupper(ch), ctx);

    return out;
  }

private:
  formatter<char> base;
};

} // namespace fmt
