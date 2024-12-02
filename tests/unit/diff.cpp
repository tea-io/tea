#pragma GCC diagnostic ignored "-Wmissing-field-initializers"

#include <catch2/catch_test_macros.hpp>
#include <cstring>
#include "../../common/diff.h"

TEST_CASE("Diff algorithm") {
    SECTION("Empty diff") {
        std::vector<WriteOperation> diffs = diff("", "");

        REQUIRE(diffs.size() == 0);
    }

    SECTION("No difference") {
        std::vector<WriteOperation> diffs = diff("Lorem ipsum dolor sit amet", "Lorem ipsum dolor sit amet");

        REQUIRE(diffs.size() == 0);
    }

    SECTION("Long diff with word change") {
        std::vector<WriteOperation> diffs = diff("Lorem ipsum dolor sit amet, consectetur adipiscing elit. Praesent pharetra neque quis augue rutrum, nec convallis eros aliquet. Praesent ac nunc mi. Ut nec lobortis nunc. Curabitur vulputate sit amet elit eget gravida. Vivamus pulvinar erat non metus eleifend test. Donec id ullamcorper nisl. Praesent venenatis varius scelerisque. Sed tempus lorem auctor lectus pretium, tempus finibus odio ultricies. Fusce in venenatis eros, id suscipit elit. Maecenas ac sem vitae eros tincidunt lacinia quis ac erat. Nunc purus nibh, vulputate sit amet mauris eu, porta egestas eros. Sed in quam ullamcorper, maximus nibh ac, accumsan nunc.", "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Praesent pharetra neque quis augue rutrum, nec convallis eros aliquet. Praesent ac nunc mi. Ut nec lobortis nunc. Curabitur vulputate sit amet elit eget gravida. Vivamus pulvinar erat non metus eleifend elementum. Donec id ullamcorper nisl. Praesent venenatis varius scelerisque. Sed tempus lorem auctor lectus pretium, tempus finibus odio ultricies. Fusce in venenatis eros, id suscipit elit. Maecenas ac sem vitae eros tincidunt lacinia quis ac erat. Nunc purus nibh, vulputate sit amet mauris eu, porta egestas eros. Sed in quam ullamcorper, maximus nibh ac, accumsan nunc.");

        // -t e +l +e +m +e +n -s t +u +m 
        
        REQUIRE(diffs.size() == 4);
        REQUIRE(diffs[0].flag() == DELETE);
        REQUIRE(diffs[0].data() == "t");
        REQUIRE(diffs[0].size() == 1);

        REQUIRE(diffs[1].flag() == APPEND);
        REQUIRE(diffs[1].data() == "lemen");
        REQUIRE(diffs[1].size() == 5);

        REQUIRE(diffs[2].flag() == DELETE);
        REQUIRE(diffs[2].data() == "s");
        REQUIRE(diffs[2].size() == 1);

        REQUIRE(diffs[3].flag() == APPEND);
        REQUIRE(diffs[3].data() == "um");
        REQUIRE(diffs[3].size() == 2);
    }

    SECTION("Insert in the middle") {
        std::vector<WriteOperation> diffs = diff("abce", "ab123ce");

        REQUIRE(diffs.size() == 1);
        REQUIRE(diffs[0].flag() == APPEND);
        REQUIRE(diffs[0].data() == "123");
        REQUIRE(diffs[0].size() == 3);
    }

    SECTION("Delete in the middle") {
        std::vector<WriteOperation> diffs = diff("abcdefg", "abfg");

        REQUIRE(diffs.size() == 1);
        REQUIRE(diffs[0].flag() == DELETE);
        REQUIRE(diffs[0].data() == "cde");
        REQUIRE(diffs[0].size() == 3);
    }

    SECTION("Trim from text start") {
        std::vector<WriteOperation> diffs = diff("abcdefg", "efg");

        REQUIRE(diffs.size() == 1);
        REQUIRE(diffs[0].flag() == DELETE);
        REQUIRE(diffs[0].data() == "abcd");
        REQUIRE(diffs[0].size() == 4);
    }

    SECTION("Trim from text end") {
        std::vector<WriteOperation> diffs = diff("abcdefg", "abc");

        REQUIRE(diffs.size() == 1);
        REQUIRE(diffs[0].flag() == DELETE);
        REQUIRE(diffs[0].data() == "defg");
        REQUIRE(diffs[0].size() == 4);
    }

}
