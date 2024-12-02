#include <catch2/catch_test_macros.hpp>
#include "../../filesystem/local_copy.h"

TEST_CASE("local_copy") {
    init_local_copy(1);
    REQUIRE(get_local_copy(1) == "");
    char data[] = "Hello, World!";
    patch_local_copy(1, data, sizeof(data), 10);
    REQUIRE(get_local_copy(1).substr(10, sizeof(data) - 1) == "Hello, World!");
    patch_local_copy(1, data, sizeof(data), 0); 
    truncate_local_copy(1, 10);
    REQUIRE(get_local_copy(1) == "Hello, Wor");
    patch_local_copy(1, data, sizeof(data), 5);
    printf("%zu\n", get_local_copy(1).size());
    std::string expected = "HelloHello, World!";
    REQUIRE(get_local_copy(1).compare(expected) == 0);
    discard_local_copy(1);
}
