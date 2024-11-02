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
    int (*open_request)(int sock, int id, OpenRequest *request);
    int (*open_response)(int sock, int id, OpenResponse *response);
    int (*release_request)(int sock, int id, ReleaseRequest *request);
    int (*release_response)(int sock, int id, ReleaseResponse *response);
    int (*read_dir_request)(int sock, int id, ReadDirRequest *request);
    int (*read_dir_response)(int sock, int id, ReadDirResponse *response);
    int (*read_request)(int sock, int id, ReadRequest *request);
    int (*read_response)(int sock, int id, ReadResponse *response);
    int (*write_request)(int sock, int id, WriteRequest *request);
    int (*write_response)(int sock, int id, WriteResponse *response);
    int (*create_request)(int sock, int id, CreateRequest *request);
    int (*create_response)(int sock, int id, CreateResponse *response);
    int (*mkdir_request)(int sock, int id, MkdirRequest *request);
    int (*mkdir_response)(int sock, int id, MkdirResponse *response);
    int (*unlink_request)(int sock, int id, UnlinkRequest *request);
    int (*unlink_response)(int sock, int id, UnlinkResponse *response);
    int (*rmdir_request)(int sock, int id, RmdirRequest *request);
    int (*rmdir_response)(int sock, int id, RmdirResponse *response);
};

int handle_recv(int sock, recv_handlers &handlers);

int full_read(int fd, char &buf, int size);
