#include "catch2/catch.hpp"
#include "cxx20/syncstream"

#include <memory>
#include <sstream>

SCENARIO("osyncstream usage", "[syncstream]") {
  GIVEN("An output stream") {
    std::ostringstream out;

    AND_GIVEN("a synchronized stream on it") {
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

  GIVEN("An output stream") {
    std::ostringstream out;

    THEN("it is empty") {
      REQUIRE(out.str() == "");
    }

    AND_GIVEN("a synchronized stream on it") {
      auto synchronized_out = std::make_unique<cxx20::osyncstream>(out);

      WHEN("the synchronized stream is destroyed") {
        synchronized_out.reset();

        THEN("the output stream is still empty") {
          REQUIRE(out.str() == "");
        }
      }
    }
  }
}
