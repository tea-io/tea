#include <cstdint>
#include <cstring>

const uint32_t POLYNOMIAL = 0xEDB88320;

uint32_t crc_table[256];

void init_crc_table() {
    for (uint32_t i = 0; i < 256; ++i) {
        uint32_t crc = i;
        for (int j = 0; j < 8; ++j) {
            if (crc & 1) {
                crc = POLYNOMIAL ^ (crc >> 1);
            } else {
                crc >>= 1;
            }
        }
        crc_table[i] = crc;
    }
}

uint32_t crc32(const char *buffer, size_t length) {
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < length; ++i) {
        uint8_t index = static_cast<uint8_t>((crc ^ buffer[i]) & 0xFF);
        crc = (crc >> 8) ^ crc_table[index];
    }
    return ~crc;
}
