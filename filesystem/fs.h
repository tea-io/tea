#pragma once
#include <fuse3/fuse.h>
#include <string>

struct config {
    std::string name;
    bool vim_mode;
};

fuse_operations get_fuse_operations(int sock, int ext_sock, config cfg);
