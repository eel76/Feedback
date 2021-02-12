#include "generator/feedback.h"

#include <unordered_map>

namespace generator::feedback {

  namespace {
    template <class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
    template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

    using responses = std::unordered_map<std::string_view, response>;

    template <std::size_t... Index> auto get_responses(std::index_sequence<Index...>) -> responses {
      return { { to_string(response{ std::in_place_index<Index> }), response{ std::in_place_index<Index> } }... };
    }

  } // namespace

  auto to_string(feedback::response response) -> std::string_view {
    return std::visit(overloaded{
                      [](feedback::none) { return "none"; },
                      [](feedback::message) { return "message"; },
                      [](feedback::warning) { return "warning"; },
                      [](feedback::error) { return "error"; },
                      },
                      response);
  }

  auto parse_response(std::string_view text) -> response {
    static responses const all_responses{ get_responses(std::make_index_sequence<std::variant_size_v<response>>{}) };
    return all_responses.at(text);
  }

  workflow::workflow(feedback::handlings handlings) : handlings_(std::move(handlings)) {
  }

  auto workflow::operator[](std::string const& type) const -> handling {
    if (auto const itr = handlings_.find(type); itr != cend(handlings_))
      return itr->second;

    if (auto const itr = handlings_.find("default"); itr != cend(handlings_))
      return itr->second;

    return {};
  }
} // namespace generator::feedback
