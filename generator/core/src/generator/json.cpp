#include "generator/json.h"

#include <nlohmann/json.hpp>

namespace generator::feedback {
  void from_json(nlohmann::json const& json, feedback::response& response) {
    response = parse_response(json.get<std::string>());
  }

  NLOHMANN_JSON_SERIALIZE_ENUM(check,
                               { { check::ALL_FILES, "all_files" },
                                 { check::ALL_LINES, "all_lines" },
                                 { check::CHANGED_FILES, "changed_files" },
                                 { check::CHANGED_LINES, "changed_lines" },
                                 { check::NO_FILES, "no_files" },
                                 { check::NO_LINES, "no_lines" } })

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
