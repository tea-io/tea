#pragma once

#include <cstddef>
#include <cstdint>

void init_crc_table();
uint32_t crc32(const char *buffer, size_t length);

void crc32_begin();
void crc32_sum(char *buffer, size_t length);
uint32_t crc32_end();
