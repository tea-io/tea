#pragma once
#include "../proto/messages.pb.h"
#include <arpa/inet.h>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <google/protobuf/message.h>

int send_message(int sock, int id, Type type, google::protobuf::Message *message);

struct recv_handlers {
    int (*init_request)(int sock, int id, InitRequest *request);
    int (*init_response)(int sock, int id, InitResponse *response);
    int (*get_attr_request)(int sock, int id, GetAttrRequest *request);
    int (*get_attr_response)(int sock, int id, GetAttrResponse *response);
};

int handle_recv(int sock, recv_handlers &handlers);

int full_read(int fd, char &buf, int size);
