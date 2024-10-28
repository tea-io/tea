#pragma once
#include <arpa/inet.h>
#include <cstddef>
#include <cstdint>
#include <cstring>

const int HEADER_SIZE = 12;

struct Header {
    int32_t size;
    int32_t id;
    int32_t type;
};

char *serialize(Header *header);

void *deserialize(char *buffer, Header *header);
