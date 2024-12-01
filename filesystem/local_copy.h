#ifndef LOCALCOPY_H
#define LOCALCOPY_H
#include <optional>
#include <string_view>

void ensure_local_copy_initialized(int socket, const char *path);
void patch_local_copy(const char *path, const char *data, size_t size, size_t offset);
void discard_local_copy(const char *path);
void truncate_local_copy(const char *path, size_t size);

std::optional<std::string_view> get_local_copy(const char *path);

#endif // LOCALCOPY_H
