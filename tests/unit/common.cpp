#include <absl/status/status.h>
#include <google/protobuf/json/json.h>
#include <google/protobuf/util/json_util.h>
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#include "../../common/header.h"
#include "../../common/tcp.h"
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
    int len = full_read(fd2, *buffer, 10);
    REQUIRE(len == 10);
    REQUIRE(strncmp(buffer, "1234567890", 10) == 0);
    close(fd1);
    close(fd2);
    res.wait();
}

TEST_CASE("send_message") {
    auto [fd1, fd2] = testing_socket();
    SECTION("protobuf") {
        InitRequest request;
        request.set_name("test");
        int err = send_message(fd1, 1, INIT_REQUEST, &request);
        REQUIRE(err > 0);
        char *buffer = new char[HEADER_SIZE + static_cast<int>(request.ByteSizeLong())];
        int len = full_read(fd2, *buffer, HEADER_SIZE + static_cast<int>(request.ByteSizeLong()));
        REQUIRE(static_cast<int>(len) == HEADER_SIZE + static_cast<int>(request.ByteSizeLong()));
        Header *header = new Header();
        deserialize(buffer, header);
        REQUIRE(header->size == (int32_t)request.ByteSizeLong());
        REQUIRE(header->id == 1);
        REQUIRE(header->type == INIT_REQUEST);
        InitRequest recv_request;
        recv_request.ParseFromArray(buffer + HEADER_SIZE, header->size);
        REQUIRE(recv_request.name() == "test");
        delete header;
        delete[] buffer;
    }
    SECTION("json"){
        DiffWriteEnable event;
        event.set_path("test");
        std::string json;
        absl::Status status= google::protobuf::json::MessageToJsonString(event, &json);
        REQUIRE(status.ok());
        int err = send_message(fd1, 1, DIFF_WRITE_ENABLE_EVENT, &event, true);
        REQUIRE(err > 0);
        char *buffer = new char[HEADER_SIZE + static_cast<int>(json.size())];
        int len = full_read(fd2, *buffer, HEADER_SIZE + static_cast<int>(json.size()));
        REQUIRE(static_cast<int>(len) == HEADER_SIZE + static_cast<int>(json.size()));
        Header *header = new Header();
        deserialize(buffer, header);
        REQUIRE(header->size == json.size());
        REQUIRE(header->id == 1);
        REQUIRE(header->type == DIFF_WRITE_ENABLE_EVENT);
        DiffWriteEnable recv_event;
        status = google::protobuf::util::JsonStringToMessage(buffer + HEADER_SIZE, &recv_event);
        REQUIRE(status.ok());
        REQUIRE(recv_event.path() == "test");
        delete header;
    };
    close(fd1);
    close(fd2);
}

TEST_CASE("handle_recv") {
    auto [fd1, fd2] = testing_socket();
    SECTION("FS proto") {
        InitRequest request;
        request.set_name("test");
        int err = send_message(fd1, 1, INIT_REQUEST, &request);
        REQUIRE(err > 0);
        recv_handlers handlers = {.fs{
            .init_request =
                [](int sock, int id, InitRequest *request) {
                    (void)sock;
                    REQUIRE(id == 1);
                    REQUIRE(request->name() == "test");
                    return 1;
                },
            .init_response = nullptr,
        }};
        int ret = handle_recv(fd2, handlers, false, FS);
        REQUIRE(ret == 1);
    }
    SECTION("EVENT json") {
        DiffWriteEnable event;
        event.set_path("test");
        int err = send_message(fd1, 1, DIFF_WRITE_ENABLE_EVENT, &event, true);
        REQUIRE(err > 0);
        recv_handlers handlers = {.event{
            .cursor_position = nullptr,
            .diff_write_enable =
                [](int sock, int id, DiffWriteEnable *request) {
                    (void)sock;
                    REQUIRE(id == 1);
                    REQUIRE(request->path() == "test");
                    return 1;
                },
            .diff_write_disable = nullptr,
        }};
        int ret = handle_recv(fd2, handlers, true, EVENT);
        REQUIRE(ret == 1);
    };
    close(fd1);
    close(fd2);
}
