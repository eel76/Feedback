#include "regex.h"
#include "catch2/catch.hpp"

SCENARIO("regex can be compiled", "[regex]")
{
  GIVEN("A pattern which matches any character")
  {
    auto const any_character_pattern = std::string{ "." };

    WHEN("it is compiled to a regex")
    {
      auto const any_regex = regex::compile(any_character_pattern);

      THEN("it doesn't match an empty text")
      {
        auto const empty_text = "";
        REQUIRE(not search(empty_text, any_regex));
      }

      THEN("it matches any non-empty text")
      {
        auto const any_text = "42";
        REQUIRE(search(any_text, any_regex));
      }
    }
    WHEN("it is captured")
    {
      auto const capture_pattern = regex::capture(any_character_pattern);

      THEN("it starts with a parenthesis")
      {
        REQUIRE(capture_pattern.front() == '(');
      }

      THEN("it ends with a parenthesis")
      {
        REQUIRE(capture_pattern.back() == ')');
      }
    }
  }
}
