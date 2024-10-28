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

TEST_CASE("full_read") {
    auto [fd1, fd2] = testing_socket();
    char buffer[10];
    send(fd1, "12345", 5, 0);
    auto res = std::async(std::launch::async, [fd1]() {
        sleep(1);
        send(fd1, "67890", 5, 0);
    });
    int len = full_read(fd2, buffer, 10);
    REQUIRE(len == 10);
    REQUIRE(strncmp(buffer, "1234567890", 10) == 0);
    close(fd1);
    close(fd2);
    res.wait();
}

TEST_CASE("send_message") {
    auto [fd1, fd2] = testing_socket();
    InitRequest request;
    request.set_error(2);
    int err = send_message(fd1, 1, INIT_REQUEST, &request);
    REQUIRE(err > 0);
    char *buffer = new char[HEADER_SIZE + static_cast<int>(request.ByteSizeLong())];
    int len = full_read(fd2, buffer, HEADER_SIZE + static_cast<int>(request.ByteSizeLong()));
    REQUIRE(static_cast<int>(len) == HEADER_SIZE + static_cast<int>(request.ByteSizeLong()));
    Header *header = new Header();
    deserialize(buffer, header);
    REQUIRE(header->size == (int32_t)request.ByteSizeLong());
    REQUIRE(header->id == 1);
    REQUIRE(header->type == INIT_REQUEST);
    InitRequest recv_request;
    recv_request.ParseFromArray(buffer + HEADER_SIZE, header->size);
    REQUIRE(recv_request.error() == 2);
    free(header);
    delete[] buffer;
    close(fd1);
    close(fd2);
}

TEST_CASE("handle_recv") {
    auto [fd1, fd2] = testing_socket();
    InitRequest request;
    request.set_error(2);
    int err = send_message(fd1, 1, INIT_REQUEST, &request);
    REQUIRE(err > 0);
    recv_handlers handlers = {
        .init_request =
            [](int sock, int id, InitRequest *request) {
                (void)sock;
                REQUIRE(id == 1);
                REQUIRE(request->error() == 2);
                return 1;
            },
        .init_response = nullptr,
    };
    int ret = handle_recv(fd2, handlers);
    REQUIRE(ret == 1);
    close(fd1);
    close(fd2);
}
