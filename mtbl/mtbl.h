/*
 * Copyright (c) 2012 by Internet Systems Consortium, Inc. ("ISC")
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef MTBL_H
#define MTBL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>
#include <stdbool.h>
#include <stdint.h>

struct mtbl_writer;
struct mtbl_writer *	mtbl_writer_init(const char *fname);

void		mtbl_writer_destroy(struct mtbl_writer **w);
void		mtbl_writer_add(struct mtbl_writer *w,
				const uint8_t *key, size_t len_key,
				const uint8_t *val, size_t len_val);

uint32_t	mtbl_crc32c(const uint8_t *buffer, size_t length);

size_t		mtbl_fixed_encode32(uint8_t *dst, uint32_t value);
size_t		mtbl_fixed_encode64(uint8_t *dst, uint64_t value);
uint32_t	mtbl_fixed_decode32(const uint8_t *ptr);
uint64_t	mtbl_fixed_decode64(const uint8_t *ptr);

unsigned	mtbl_varint_length(uint64_t v);
unsigned	mtbl_varint_length_packed(const uint8_t *buf);
size_t		mtbl_varint_encode32(uint8_t *ptr, uint32_t value);
size_t		mtbl_varint_encode64(uint8_t *ptr, uint64_t value);
size_t		mtbl_varint_decode32(uint8_t *ptr, uint32_t *value);
size_t		mtbl_varint_decode64(uint8_t *ptr, uint64_t *value);

#ifdef __cplusplus
}
#endif

#endif /* MTBL_H */
