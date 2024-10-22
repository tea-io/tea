#pragma once
#include <fuse3/fuse.h>

fuse_operations get_fuse_operations(int sock);
