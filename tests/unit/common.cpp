#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#include "../../common/header.h"
#include "../../common/io.h"
#include <catch2/catch_test_macros.hpp>
#include <cstring>
#include <future>

TEST_CASE("Header serialization") {
    Header header = {1, 2, 3};
    char *buffer = serialize(&header);
    REQUIRE(buffer != nullptr);
    char expected[] = {0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x03};
    for (int i = 0; i < HEADER_SIZE; i++) {
        REQUIRE(buffer[i] == expected[i]);
    }
    free(buffer);
}

TEST_CASE("Header deserialization") {
    char buffer[] = {0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x03};
    Header *header = new Header();
    deserialize(buffer, header);
    REQUIRE(header != nullptr);
    REQUIRE(header->size == 1);
    REQUIRE(header->id == 2);
    REQUIRE(header->type == 3);
    free(header);
}

static std::pair<int, int> testing_socket() {
    int fd[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, fd) < 0) {
        return std::pair(-1, -1);
    }
    return std::pair(fd[0], fd[1]);
}
