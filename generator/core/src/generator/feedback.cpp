#include "generator/feedback.h"

namespace generator::feedback {

  namespace {
    template <class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
    template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;
  } // namespace

  auto to_string(feedback::check check) -> std::string_view {
    return std::visit(overloaded{ [&](feedback::nothing) { return "nothing"; },
                                  [&](feedback::changed_lines) { return "changed_lines"; },
                                  [&](feedback::changed_files) { return "changed_files"; },
                                  [&](feedback::everything) { return "everything"; } },
                      check);
  }

  auto to_string(feedback::response response) -> std::string_view {
    return std::visit(overloaded{ [](feedback::none) { return "none"; }, [](feedback::message) { return "message"; },
                                  [](feedback::warning) { return "warning"; }, [](feedback::error) { return "error"; } },
                      response);
  }

  workflow::workflow(feedback::handlings handlings) noexcept : handlings_(std::move(handlings)) {
  }

  auto workflow::operator[](std::string const& type) const noexcept -> handling {
    if (auto const itr = handlings_.find(type); itr != cend(handlings_))
      return itr->second;

    if (auto const itr = handlings_.find("default"); itr != cend(handlings_))
      return itr->second;

    return {};
  }
} // namespace generator::feedback
