#include "generator/json.h"

#include <nlohmann/json.hpp>

namespace generator::feedback {
  namespace {
    template <class V, std::size_t... Index>
    auto get_variant_values(std::index_sequence<Index...>) -> std::unordered_map<std::string_view, V> {
      return { { to_string(V{ std::in_place_index<Index> }), V{ std::in_place_index<Index> } }... };
    }

    template <class V> auto all_variant_values() {
      return get_variant_values<V>(std::make_index_sequence<std::variant_size_v<V>>{});
    }
  } // namespace

  void from_json(nlohmann::json const& json, feedback::check& check) {
    static auto const all_checks{ all_variant_values<feedback::check>() };
    check = all_checks.at(json.get<std::string>());
  }

  void from_json(nlohmann::json const& json, feedback::response& response) {
    static auto const all_responses{ all_variant_values<feedback::response>() };
    response = all_responses.at(json.get<std::string>());
  }

  void from_json(nlohmann::json const& json, feedback::handling& handling) {
    handling.check    = json.value("check", handling.check);
    handling.response = json.value("response", handling.response);
  }

  void from_json(nlohmann::json const& json, feedback::rule& rule) {
    rule.type          = json.at("type");
    rule.summary       = json.at("summary");
    rule.rationale     = json.value("rationale", "N/A");
    rule.workaround    = json.value("workaround", "N/A");
    rule.matched_files = regex::capture(json.value("matched_files", ".*"));
    rule.ignored_files = regex::capture(json.value("ignored_files", "^$"));
    rule.matched_text  = regex::capture(json.at("matched_text").get<std::string>());
    rule.ignored_text  = regex::capture(json.value("ignored_text", "^$"));
    rule.marked_text   = regex::capture(json.value("marked_text", ".*"));
  }
} // namespace generator::feedback

namespace generator::json {

  auto parse_rules(std::string_view json) -> feedback::rules {
    return nlohmann::json::parse(json).get<feedback::rules>();
  }
  auto parse_workflow(std::string_view json) -> feedback::workflow {
    return feedback::workflow{ nlohmann::json::parse(json).get<feedback::handlings>() };
  }
} // namespace generator::json
