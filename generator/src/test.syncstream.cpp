#include "catch2/catch.hpp"
#include "cxx20/syncstream"

#include <memory>
#include <sstream>

SCENARIO("osyncstream usage", "[syncstream]") {
  GIVEN("A synchronized output stream") {
    std::ostringstream out;

    auto synchronized_out = std::make_unique<cxx20::osyncstream>(out);

    WHEN("something is written to it") {
      *synchronized_out << "something";

      THEN("something is not emitted yet") {
        REQUIRE(out.str() == "");

        AND_WHEN("emit is requested") {
          synchronized_out->emit();

          THEN("something is emitted") {
            REQUIRE(out.str() == "something");

            AND_WHEN("something else is written to it") {
              *synchronized_out << " else";

              THEN("something else is not emitted yet") {
                REQUIRE(out.str() == "something");

                AND_WHEN("it is destroyed") {
                  synchronized_out.reset();

                  THEN("something else is emitted") {
                    REQUIRE(out.str() == "something else");
                  }
                }
              }
            }
          }
        }
      }
    }
  }
}
