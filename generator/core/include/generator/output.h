#pragma once
#include "generator/feedback.h"
#include "generator/text.h"

#include <future>
#include <string>
#include <string_view>

namespace generator::output {
  struct header {
    std::string_view const&                       rules_origin;
    std::shared_future<feedback::rules> const&    shared_rules;
    std::shared_future<feedback::workflow> const& shared_workflow;
  };

  struct source {
    std::string const& filename;
  };

  struct match {
    std::string_view const& id;
    int const&              line_number;
    text::excerpt const&    highlighting;
  };

  void print(std::ostream& out, output::header header);
  void print(std::ostream& out, output::source source);
  void print(std::ostream& out, output::match match);
} // namespace generator::output
