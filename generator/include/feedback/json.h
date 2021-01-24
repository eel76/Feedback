#pragma once
#include "feedback/control.h"

#include <string_view>

namespace feedback::json {
  auto parse_rules(std::string_view json) -> control::rules;
  auto parse_workflow(std::string_view json) -> control::workflow;

  auto to_string(control::response response) -> std::string;
} // namespace feedback::json
