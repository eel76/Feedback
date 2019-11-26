#pragma once

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wtype-limits"
#endif

#include <fmt/format.h>

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

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
    for (auto it = text.str.begin(); it != text.str.end(); ++it)
    {
      switch (*it)
      {
      case '\n':
        out = base.format('\\', ctx);
        out = base.format('n', ctx);
        break;
      case '\\':
        [[fallthrough]];
      case '\"':
        out = base.format('\\', ctx);
        [[fallthrough]];
      default:
        out = base.format(*it, ctx);
        break;
      }
    }

    return out;
  }

private:
  formatter<char> base;
};

} // namespace fmt
