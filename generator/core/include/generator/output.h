#pragma once
#include "generator/feedback.h"
#include "generator/scm.h"

#include <future>
#include <iosfwd>
#include <string>
#include <string_view>
#include <vector>

namespace generator::output {
  struct matches {
    std::string_view const&                             rules_origin;
    std::shared_future<feedback::rules> const&          shared_rules;
    std::shared_future<std::vector<std::string>> const& shared_sources;
    std::shared_future<feedback::workflow> const&       shared_workflow;
    std::shared_future<scm::diff> const&                shared_diff;
  };

  void print(std::ostream& out, output::matches matches);
} // namespace generator::output
