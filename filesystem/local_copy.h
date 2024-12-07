#pragma once
#include <optional>
#include <string_view>

void init_copy(std::string path);
void patch_copy(std::string path, const char *data, size_t size, size_t offset);
void discard_copy(std::string path);
void truncate_copy(std::string, size_t size);

std::string get_copy(std::string path);
std::string get_copy(std::string path, size_t size, size_t offset);
