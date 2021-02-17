#pragma once
#include "generator/feedback.h"
#include "generator/scm.h"

#include <filesystem>
#include <future>
#include <iosfwd>
#include <vector>

namespace generator::output {
  struct matches {
    std::filesystem::path const&                                  rules_origin;
    std::shared_future<feedback::rules> const&                    shared_rules;
    std::shared_future<std::vector<std::filesystem::path>> const& shared_sources;
    std::shared_future<feedback::workflow> const&                 shared_workflow;
    std::shared_future<scm::diff> const&                          shared_diff;
  };

  struct stats {
    void process(std::string_view source) {
      ++sources;
      bytes += source.length();
    }

    void merge(stats const& other) {
      sources += other.sources;
      bytes += other.bytes;
    }

    size_t sources{ 0 };
    size_t bytes{ 0 };
  };

  auto print(std::ostream& out, output::matches matches, stats merged_stats = {}) -> stats;
} // namespace generator::output
