#pragma once
#include "feedback/control.h"

#include <future>
#include <string>
#include <string_view>

namespace feedback::output {
  struct header {
    std::shared_future<control::rules> const&    shared_rules;
    std::shared_future<control::workflow> const& shared_workflow;
    std::string_view const&                      rules_origin;
  };

  struct source {
    std::string const& filename;
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
    std::string_view const& id;
    int const&              line_number;
    excerpt const&          highlighting;
  };

  void print(std::ostream& out, output::header header);
  void print(std::ostream& out, output::source source);
  void print(std::ostream& out, output::match match);
} // namespace feedback::output
