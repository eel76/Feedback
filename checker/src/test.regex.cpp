#include "regex.h"

#include "catch2/catch.hpp"

#include <string>

SCENARIO("regex can be compiled", "[regex]")
{
  GIVEN("A simple string pattern")
  {
    auto const any_pattern = std::string{"."};

    WHEN("a regex is compiled")
    {
      auto const any_regex = regex::precompiled{ any_pattern };

      THEN("its state is valid")
      {
        REQUIRE(search("test", any_regex) == true);
      }
    }
  }
}

