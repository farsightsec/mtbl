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

#ifndef MTBL_PRIVATE_H
#define MTBL_PRIVATE_H

#ifdef HAVE_ENDIAN_H
# include <endian.h>
#else
# ifdef HAVE_SYS_ENDIAN_H
#  include <sys/endian.h>
# endif
#endif

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mtbl.h"

#include <snappy-c.h>
#include <zlib.h>

#include "librsf/my_alloc.h"

#define MTBL_MAGIC			0x77846676
#define MTBL_TRAILER_SIZE		512

#define DEFAULT_COMPRESSION_TYPE	MTBL_COMPRESSION_ZLIB
#define DEFAULT_BLOCK_RESTART_INTERVAL	16
#define DEFAULT_BLOCK_SIZE		8192
#define MIN_BLOCK_SIZE			1024

#define DEFAULT_SORTER_TEMP_DIR		"/var/tmp"
#define DEFAULT_SORTER_MEMORY		1073741824
#define MIN_SORTER_MEMORY		10485760
#define INITIAL_SORTER_VEC_SIZE		131072

#define DEFAULT_FILESET_RELOAD_FREQ	60

/* types */

struct block;
struct block_builder;
struct block_iter;
struct trailer;

/* block */

struct block *block_init(uint8_t *data, size_t size, bool needs_free);
void block_destroy(struct block **);

struct block_iter *block_iter_init(struct block *);
void block_iter_destroy(struct block_iter **);
bool block_iter_valid(const struct block_iter *);
void block_iter_seek_to_first(struct block_iter *);
void block_iter_seek_to_last(struct block_iter *);
void block_iter_seek(struct block_iter *, const uint8_t *key, size_t key_len);
bool block_iter_next(struct block_iter *);
void block_iter_prev(struct block_iter *);
bool block_iter_get(struct block_iter *,
	const uint8_t **key, size_t *key_len,
	const uint8_t **val, size_t *val_len);

/* block builder */

struct block_builder *block_builder_init(size_t block_restart_interval);
size_t block_builder_current_size_estimate(struct block_builder *);
void block_builder_destroy(struct block_builder **);
void block_builder_finish(struct block_builder *,
	uint8_t **buf, size_t *bufsz);
void block_builder_reset(struct block_builder *);
void block_builder_add(struct block_builder *,
	const uint8_t *key, size_t len_key,
	const uint8_t *val, size_t len_val);
bool block_builder_empty(struct block_builder *);

/* trailer */

struct trailer {
	uint64_t	index_block_offset;
	uint64_t	data_block_size;
	uint64_t	compression_algorithm;
	uint64_t	count_entries;
	uint64_t	count_data_blocks;
	uint64_t	bytes_data_blocks;
	uint64_t	bytes_index_block;
	uint64_t	bytes_keys;
	uint64_t	bytes_values;
};

void trailer_write(struct trailer *t, uint8_t *buf);
bool trailer_read(const uint8_t *buf, struct trailer *t);

/* misc */

static inline int
bytes_compare(const uint8_t *a, size_t len_a,
	      const uint8_t *b, size_t len_b)
{
	size_t len = len_a > len_b ? len_b : len_a;
	int ret = memcmp(a, b, len);
	if (ret == 0) {
		if (len_a < len_b) {
			return (-1);
		} else if (len_a == len_b) {
			return (0);
		} else if (len_a > len_b) {
			return (1);
		}
	}
	return (ret);
}

#endif /* MTBL_PRIVATE_H */
