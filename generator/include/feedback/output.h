#pragma once
#include "feedback/control.h"

#include <future>
#include <string_view>

namespace feedback::output {
  struct header {
    std::shared_future<control::rules> const&    shared_rules;
    std::shared_future<control::workflow> const& shared_workflow;
    std::string_view                             rules_origin;
  };

  void print(std::ostream& out, output::header const& header);
} // namespace feedback::output
