#pragma once
#include "../proto/messages.pb.h"
#include "handlers.h"
#include <arpa/inet.h>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <google/protobuf/message.h>

int listen(int port);

int send_message(int sock, int id, Type type, google::protobuf::Message *message);
int send_message(int sock, int id, Type type, google::protobuf::Message *message, bool json);

int handle_recv(int sock, recv_handlers &handlers, bool json, socket_type type);
void recv_therad(int fd, recv_handlers handlers, bool json, socket_type type);
int accept_connections(int sock, recv_handlers handlers, bool json, socket_type type);
int close_connections();

int full_read(int fd, char &buf, int size);
