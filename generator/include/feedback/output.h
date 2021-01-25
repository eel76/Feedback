#pragma once
#include "feedback/control.h"

#include <future>
#include <string>
#include <string_view>

namespace feedback::output {
  struct header {
    std::shared_future<control::rules> const&    shared_rules;
    std::shared_future<control::workflow> const& shared_workflow;
    std::string_view                             rules_origin;
  };

  struct source {
    std::string filename;
  };

  class excerpt {
  public:
    excerpt(std::string_view text, std::string_view match);

  public:
    std::string_view first_line;
    std::string      indentation;
    std::string      annotation;
  };

  struct match {
    std::string_view id;
    int              line_number;
    excerpt          highlighting;
  };

  void print(std::ostream& out, output::header const& header);
  void print(std::ostream& out, output::source const& source);
  void print(std::ostream& out, output::match const& match);
} // namespace feedback::output
