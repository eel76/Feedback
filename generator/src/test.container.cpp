#include "generator/container.h"

#include "catch2/catch.hpp"

SCENARIO("interval map usage", "[container]") {
  GIVEN("An interval map from integers to booleans") {
    auto const map = generator::container::interval_map<int, bool>{ false };

    WHEN("it is default constructed") {
      THEN("it is canonical") {
        REQUIRE(map.is_canonical());
      }
      THEN("any value is false") {
        auto const any_value = 42;
        REQUIRE(map[any_value] == false);
      }
    }
  }
}
