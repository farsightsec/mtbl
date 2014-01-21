#ifndef MY_CRC32C_H
#define MY_CRC32C_H

#include <stdint.h>

typedef uint32_t (*my_crc32c_fp)(const uint8_t *buffer, size_t length);

extern my_crc32c_fp my_crc32c;

#endif /* MY_CRC32C_H */
