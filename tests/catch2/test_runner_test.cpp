#include <catch2/catch_test_macros.hpp>

TEST_CASE("A basic test that confirms that tests are run") {
    SECTION("Basic operations") {
        REQUIRE(0.1 + 0.2 != 0.3);
    }
}