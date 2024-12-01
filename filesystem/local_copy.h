#pragma once
#include <optional>
#include <string_view>

// fd is the fh field from the fuse_file_info struct
void init_local_copy(int fd);
void patch_local_copy(int fd, const char *data, size_t size, size_t offset);
void discard_local_copy(int fd);
void truncate_local_copy(int fd, size_t size);

std::string get_local_copy(int fd);
std::string get_local_copy(int fd, size_t size, size_t offset);
