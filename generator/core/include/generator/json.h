#pragma once
#include "generator/feedback.h"

#include <string_view>

namespace generator::json {
  auto parse_rules(std::string_view json) -> feedback::rules;
  auto parse_workflow(std::string_view json) -> feedback::workflow;

  auto to_string(feedback::response response) -> std::string;
} // namespace generator::json
