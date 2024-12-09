#pragma once

#include <cstddef>
#include <cstdint>

void init_crc_table();
uint32_t crc32(const char *buffer, size_t length);
