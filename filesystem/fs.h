#pragma once
#include <fuse3/fuse.h>
#include <gnutls/gnutls.h>
#include <string>

struct config {
    std::string name;
};

fuse_operations get_fuse_operations(int sock, config cfg, gnutls_session_t ssl);
