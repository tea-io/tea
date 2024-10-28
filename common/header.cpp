#include "header.h"
#include <cstdint>

char *serialize(Header *header) {
    char *buf = new char[sizeof(Header)];
    uint32_t nsize = htonl(header->size);
    uint32_t nid = htonl(header->id);
    uint32_t ntype = htonl(header->type);
    memcpy(buf, &nsize, sizeof(int32_t));
    memcpy(buf + sizeof(int32_t), &nid, sizeof(int32_t));
    memcpy(buf + sizeof(int32_t) + sizeof(int32_t), &ntype, sizeof(int32_t));
    return buf;
}

void *deserialize(char *buffer, Header *header) {
    uint32_t nsize;
    uint32_t nid;
    uint32_t ntype;
    memcpy(&nsize, buffer, sizeof(int32_t));
    memcpy(&nid, buffer + sizeof(int32_t), sizeof(int32_t));
    memcpy(&ntype, buffer + sizeof(int32_t) + sizeof(int32_t), sizeof(int32_t));
    header->size = ntohl(nsize);
    header->id = ntohl(nid);
    header->type = ntohl(ntype);
    return header;
}
