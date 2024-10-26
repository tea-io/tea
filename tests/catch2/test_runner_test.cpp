#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

TEST_CASE("A basic test that confirms that tests are, in fact, run") {
    SECTION("Basic operations") {
        REQUIRE_THAT(0.1 + 0.2, Catch::Matchers::WithinRel(0.3, 0.001));
    }
}