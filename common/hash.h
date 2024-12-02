#pragma once

#include <cstddef>

void init_crc_table();
uint32_t crc32(const char *buffer, size_t length);
