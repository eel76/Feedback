#include "feedback/json.h"

#include <nlohmann/json.hpp>

namespace feedback::control {
  NLOHMANN_JSON_SERIALIZE_ENUM(
  response,
  { { response::NONE, "none" }, { response::MESSAGE, "message" }, { response::WARNING, "warning" }, { response::ERROR, "error" } })

  NLOHMANN_JSON_SERIALIZE_ENUM(check,
                               { { check::ALL_FILES, "all_files" },
                                 { check::ALL_LINES, "all_lines" },
                                 { check::CHANGED_FILES, "changed_files" },
                                 { check::CHANGED_LINES, "changed_lines" },
                                 { check::NO_FILES, "no_files" },
                                 { check::NO_LINES, "no_lines" } })

  void from_json(nlohmann::json const& json, control::handling& handling) {
    handling.check    = json.value("check", handling.check);
    handling.response = json.value("response", handling.response);
  }

  void from_json(nlohmann::json const& json, control::rule& rule) {
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
} // namespace feedback::control

namespace feedback::json {

  auto parse_rules(std::string_view json) -> control::rules {
    return nlohmann::json::parse(json).get<control::rules>();
  }
  auto parse_workflow(std::string_view json) -> control::workflow {
    return nlohmann::json::parse(json).get<control::workflow>();
  }
  auto to_string(control::response response) -> std::string {
    auto const dump = nlohmann::json{ response }.dump();
    return dump.substr(2, dump.length() - 4);
  }
} // namespace feedback::json
