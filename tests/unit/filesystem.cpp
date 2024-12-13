#include <catch2/catch_test_macros.hpp>
#include "../../filesystem/local_copy.h"

TEST_CASE("local_copy") {
    REQUIRE(get_copy("test") == "");
    char data[] = "Hello, World!";
    patch_copy("test", data, sizeof(data), 10);
    REQUIRE(get_copy("test").substr(10, sizeof(data) - 1) == "Hello, World!");
    patch_copy("test", data, sizeof(data), 0); 
    truncate_copy("test", 10);
    REQUIRE(get_copy("test") == "Hello, Wor");
    patch_copy("test", data, sizeof(data), 5);
    printf("%zu\n", get_copy("test").size());
    std::string expected = "HelloHello, World!";
    REQUIRE(get_copy("test").compare(expected) == 0);
    discard_copy("test");
}

