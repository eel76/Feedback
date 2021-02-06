#include "generator/feedback.h"

namespace generator::feedback {

  workflow::workflow(feedback::handlings handlings) : handlings_(std::move(handlings)) {
  }

  auto workflow::operator[](std::string const& type) const -> handling {
    if (auto const itr = handlings_.find(type); itr != cend(handlings_))
      return itr->second;

    if (auto const itr = handlings_.find("default"); itr != cend(handlings_))
      return itr->second;

    return {};
  }
} // namespace generator::feedback
