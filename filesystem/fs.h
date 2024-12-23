#pragma once
#include <fuse3/fuse.h>
#include <openssl/crypto.h>
#include <string>

struct config {
    std::string name;
};

fuse_operations get_fuse_operations(int sock, config cfg, SSL *ssl);
