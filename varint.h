#ifndef MY_VARINT_H
#define MY_VARINT_H

#include <stddef.h>
#include <stdint.h>

unsigned varint_length(uint64_t v);
unsigned varint_length_packed(const uint8_t *buf, size_t len_buf);
size_t varint_encode32(uint8_t *ptr, uint32_t value);
size_t varint_encode64(uint8_t *ptr, uint64_t value);
size_t varint_decode32(const uint8_t *ptr, uint32_t *value);
size_t varint_decode64(const uint8_t *ptr, uint64_t *value);

#endif /* MY_VARINT_H */
