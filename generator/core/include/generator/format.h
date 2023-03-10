#pragma once
#include <fmt/format.h>

#include <iterator>
#include <string_view>

namespace generator::format {

  template <class Stream, class... Args> void print(Stream&& out, std::string_view fmt, Args&&... args) {
    fmt::format_to(std::ostream_iterator<char>(out), fmt, std::forward<Args>(args)...);
  }

  struct as_compiler_message {
    std::string_view str;
  };
} // namespace generator::format

namespace fmt {

  template <> struct formatter<generator::format::as_compiler_message> {
    template <typename ParseContext> constexpr auto parse(ParseContext& ctx) {
      return ctx.begin();
    }

    template <typename FormatContext>
    auto format(generator::format::as_compiler_message const& text, FormatContext& ctx) {
      auto out = ctx.out();

      bool indent{ false };

      for (auto const& ch : text.str) {
        if (indent) {
          out    = base.format(' ', ctx);
          out    = base.format(' ', ctx);
          out    = base.format(' ', ctx);
          out    = base.format(' ', ctx);
          out    = base.format(' ', ctx);
          out    = base.format(' ', ctx);
          out    = base.format('|', ctx);
          out    = base.format(' ', ctx);
          indent = false;
        }

        switch (ch) {
        case '\n':
          out    = base.format('\\', ctx);
          out    = base.format('n', ctx);
          indent = true;
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
} // namespace fmt
