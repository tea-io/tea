#include "../../server/ot.h"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("OT tranform") {
    SECTION("replace to replace") {
        char data[] = "hello";
        WriteOperation bef;
        bef.set_flag(REPLACE);
        bef.set_offset(5);
        bef.set_data(data);
        bef.set_size(5);

        WriteOperation req;
        req.set_flag(REPLACE);
        req.set_offset(5);
        req.set_data("world");
        req.set_size(5);

        transform(&req, &bef);

        REQUIRE(req.offset() == 5);
        REQUIRE(req.size() == 5);
    }
    SECTION("replace to append") {
        char data[] = "hello";
        WriteOperation bef;
        bef.set_flag(REPLACE);
        bef.set_offset(5);
        bef.set_data(data);
        bef.set_size(5);

        WriteOperation req;
        req.set_flag(APPEND);
        req.set_offset(5);
        req.set_data("world");
        req.set_size(5);

        transform(&req, &bef);

        REQUIRE(req.offset() == 5);
        REQUIRE(req.size() == 5);
    }
    SECTION("replace to delete") {
        char data[] = "hello";
        WriteOperation bef;
        bef.set_flag(REPLACE);
        bef.set_offset(5);
        bef.set_data(data);
        bef.set_size(5);

        WriteOperation req;
        req.set_flag(DELETE);
        req.set_offset(5);
        req.set_size(5);

        transform(&req, &bef);

        REQUIRE(req.offset() == 5);
        REQUIRE(req.size() == 5);
    }
    SECTION("append to replace") {
        char data[] = "hello";
        WriteOperation bef;
        bef.set_flag(APPEND);
        bef.set_offset(5);
        bef.set_data(data);
        bef.set_size(5);

        WriteOperation req;
        req.set_flag(REPLACE);
        req.set_offset(5);
        req.set_data("world");
        req.set_size(5);

        transform(&req, &bef);

        REQUIRE(req.offset() == 10);
        REQUIRE(req.size() == 5);
    }

    SECTION("append to append") {
        char data[] = "hello";
        WriteOperation bef;
        bef.set_flag(APPEND);
        bef.set_offset(5);
        bef.set_data(data);
        bef.set_size(5);

        WriteOperation req;
        req.set_flag(APPEND);
        req.set_offset(5);
        req.set_data("world");
        req.set_size(5);

        transform(&req, &bef);

        REQUIRE(req.offset() == 10);
        REQUIRE(req.size() == 5);
    }

    SECTION("append to delete") {
        char data[] = "hello";
        WriteOperation bef;
        bef.set_flag(APPEND);
        bef.set_offset(5);
        bef.set_data(data);
        bef.set_size(5);

        WriteOperation req;
        req.set_flag(DELETE);
        req.set_offset(5);
        req.set_size(5);

        transform(&req, &bef);

        REQUIRE(req.offset() == 10);
        REQUIRE(req.size() == 5);

    }

    SECTION("delete to append") {
        SECTION("colision") {
            WriteOperation bef;
            bef.set_flag(DELETE);
            bef.set_offset(5);
            bef.set_size(10);

            WriteOperation req;
            req.set_flag(APPEND);
            req.set_offset(7);
            req.set_data("world");
            req.set_size(5);

            transform(&req, &bef);

            REQUIRE(req.offset() == 5);
            REQUIRE(req.size() == 5);
        }

        SECTION("no colision") {
            WriteOperation bef;
            bef.set_flag(DELETE);
            bef.set_offset(5);
            bef.set_size(10);

            WriteOperation req;
            req.set_flag(APPEND);
            req.set_offset(20);
            req.set_data("world");
            req.set_size(5);

            transform(&req, &bef);

            REQUIRE(req.offset() == 10);
        }

    }

    SECTION("delete to replace") {
        SECTION("colision") {
            WriteOperation bef;
            bef.set_flag(DELETE);
            bef.set_offset(5);
            bef.set_size(10);

            WriteOperation req;
            req.set_flag(REPLACE);
            req.set_offset(7);
            req.set_data("world");
            req.set_size(5);

            transform(&req, &bef);

            REQUIRE(req.offset() == 0);
            REQUIRE(req.size() == 0);
        }

        SECTION("no colision") {
            WriteOperation bef;
            bef.set_flag(DELETE);
            bef.set_offset(5);
            bef.set_size(10);

            WriteOperation req;
            req.set_flag(REPLACE);
            req.set_offset(20);
            req.set_data("world");
            req.set_size(5);

            transform(&req, &bef);

            REQUIRE(req.offset() == 10);
        }
    }

    SECTION("delete to delete") {
        SECTION("colision - right side") {
            WriteOperation bef;
            bef.set_flag(DELETE);
            bef.set_offset(5);
            bef.set_size(10);

            WriteOperation req;
            req.set_flag(DELETE);
            req.set_offset(7);
            req.set_size(10);

            transform(&req, &bef);

            REQUIRE(req.offset() == 5);
            REQUIRE(req.size() == 2);
        }

        SECTION("colision - left side") {
            WriteOperation bef;
            bef.set_flag(DELETE);
            bef.set_offset(5);
            bef.set_size(10);

            WriteOperation req;
            req.set_flag(DELETE);
            req.set_offset(2);
            req.set_size(5);

            transform(&req, &bef);

            REQUIRE(req.offset() == 2);
            REQUIRE(req.size() == 3);
        }

        SECTION("no colision") {
            WriteOperation bef;
            bef.set_flag(DELETE);
            bef.set_offset(5);
            bef.set_size(10);

            WriteOperation req;
            req.set_flag(DELETE);
            req.set_offset(20);
            req.set_size(5);

            transform(&req, &bef);

            REQUIRE(req.offset() == 10);
        }
    }
}
