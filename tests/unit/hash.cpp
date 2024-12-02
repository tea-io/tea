#pragma GCC diagnostic ignored "-Wmissing-field-initializers"

#include <catch2/catch_test_macros.hpp>
#include "../../common/hash.h"

#include <cstring>

TEST_CASE("CRC32 algorithm") {
    init_crc_table();

    SECTION("Empty buffer") {
        uint32_t crc = crc32("", 0);

        REQUIRE(crc == 0x00000000);
    }

    SECTION("Check same buffer") {
        std::string buffer = "Hello, World!";
        uint32_t crc = crc32(buffer.c_str(), buffer.size());
        
        std::string buffer2 = "Hello, World!";
        uint32_t crc2 = crc32(buffer2.c_str(), buffer2.size());
        REQUIRE(crc == crc2);
    }

    SECTION("Check different buffer") {
        std::string buffer = "Hello, World!";
        uint32_t crc = crc32(buffer.c_str(), buffer.size());

        std::string buffer2 = "Hello, World?";
        uint32_t crc2 = crc32(buffer2.c_str(), buffer2.size());
        REQUIRE(crc != crc2);
    }

    SECTION("Compare to expected") {
        std::string buffer = "Hello, World!";
        uint32_t crc = crc32(buffer.c_str(), buffer.size());

        REQUIRE(crc == 0xEC4AC3D0);
    }
}
