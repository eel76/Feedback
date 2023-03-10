#include "catch2/catch.hpp"
#include "generator/regex.h"

SCENARIO("regex tests", "[regex]") {
  GIVEN("A pattern which matches any character") {
    auto const any_character_pattern = ".";

    WHEN("it is compiled to a regex") {
      auto const any_regex = generator::regex::compile(any_character_pattern);

      THEN("it doesn't match an empty text") {
        auto const empty_text = "";
        REQUIRE(not any_regex.matches(empty_text));

        AND_THEN("it matches a non-empty text") {
          auto const non_empty_text = "42";
          REQUIRE(any_regex.matches(non_empty_text));
        }
      }
    }
  }
  GIVEN("A capture regex") {
    auto const regex_with_a_capture = generator::regex::capture(".");

    WHEN("it matches some string") {
      auto const some_string = "test";

      if (regex_with_a_capture.matches(some_string)) {
        THEN("it can capture something") {
          auto something = std::string_view{};

          REQUIRE(regex_with_a_capture.matches(some_string, { &something }));
          REQUIRE(regex_with_a_capture.matches(some_string, { nullptr }));
        }
      }
    }
  }

  GIVEN("A regex which matches a name") {
    auto const name_pattern = generator::regex::compile("([a-zA-Z]+)");

    WHEN("it is applied to a text with three names") {
      auto const three_names = "Johann Sebastian Bach";

      THEN("it matches exactly three times") {
        auto remaining = std::string_view{ three_names };

        REQUIRE(name_pattern.find(remaining, nullptr, nullptr, &remaining));
        REQUIRE(name_pattern.find(remaining, nullptr, nullptr, &remaining));
        REQUIRE(name_pattern.find(remaining, nullptr, nullptr, &remaining));
        REQUIRE(not name_pattern.find(remaining, nullptr, nullptr, &remaining));
      }
    }
  }

  GIVEN("A regex which matches all lines") {
    auto const all_lines_pattern = generator::regex::compile("((?:.|\n)*)");

    WHEN("it is applied to a multiline text") {
      auto const text = R"(first line
second line
)";

      THEN("it matches the whole text") {
        auto match = generator::regex::match{};

        REQUIRE(all_lines_pattern.find(text, &match, nullptr, nullptr));
        REQUIRE(match == text);
      }
    }
  }

  GIVEN("A regex which matches a single line") {
    auto const single_line_pattern = generator::regex::compile("(.*)");

    WHEN("it is applied to a multiline text") {
      auto const text = R"(first line
second line
)";

      THEN("it doesn't match the whole text") {
        auto match = generator::regex::match{};

        REQUIRE(single_line_pattern.find(text, &match, nullptr, nullptr));
        REQUIRE(match != text);
      }
    }
  }
}
