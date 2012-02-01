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

typedef enum {
	MTBL_COMP_NONE = 0,
	MTBL_COMP_SNAPPY = 1,
	MTBL_COMP_ZLIB = 2
} mtbl_comp_type;

/* exported types */

struct mtbl_writer_options;

/* writer */
struct mtbl_writer;
struct mtbl_writer *	mtbl_writer_init(const char *fname);
void			mtbl_writer_destroy(struct mtbl_writer **w);
void			mtbl_writer_add(struct mtbl_writer *w,
					const uint8_t *key, size_t len_key,
					const uint8_t *val, size_t len_val);

/* writer options */

struct mtbl_writer_options * mtbl_writer_options_init(void);
void mtbl_writer_options_destroy(struct mtbl_writer_options **);
void mtbl_writer_options_set_compression(
	struct mtbl_writer_options *,
	mtbl_comp_type);
void mtbl_writer_options_set_block_size(
	struct mtbl_writer_options *,
	size_t block_size);
void mtbl_writer_options_set_block_restart_interval(
	struct mtbl_writer_options *,
	size_t block_restart_interval);

/* memtable */

struct mtbl_memtable;
struct mtbl_memtable *	mtbl_memtable_init(void);
void			mtbl_memtable_destroy(struct mtbl_memtable **m);
void			mtbl_memtable_add(struct mtbl_memtable *m,
					  const uint8_t *key, size_t len_key,
					  const uint8_t *val, size_t len_val);
void			mtbl_memtable_get(struct mtbl_memtable *m, size_t idx,
					  uint8_t **key, size_t *len_key,
					  uint8_t **val, size_t *len_val);
size_t			mtbl_memtable_size(struct mtbl_memtable *m);
void			mtbl_memtable_finish(struct mtbl_memtable *m);

/* crc32c */
uint32_t	mtbl_crc32c(const uint8_t *buffer, size_t length);

/* fixed */
size_t		mtbl_fixed_encode32(uint8_t *dst, uint32_t value);
size_t		mtbl_fixed_encode64(uint8_t *dst, uint64_t value);
uint32_t	mtbl_fixed_decode32(const uint8_t *ptr);
uint64_t	mtbl_fixed_decode64(const uint8_t *ptr);

/* varint */
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
