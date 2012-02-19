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
	MTBL_COMPRESSION_NONE = 0,
	MTBL_COMPRESSION_SNAPPY = 1,
	MTBL_COMPRESSION_ZLIB = 2
} mtbl_compression_type;

typedef enum {
	mtbl_res_failure = 0,
	mtbl_res_success = 1
} mtbl_res;

/* exported types */

struct mtbl_iter;
struct mtbl_source;

struct mtbl_reader;
struct mtbl_reader_options;
struct mtbl_writer;
struct mtbl_writer_options;

struct mtbl_merger;
struct mtbl_merger_options;
struct mtbl_sorter;
struct mtbl_sorter_options;

typedef void (*mtbl_merge_func)(void *clos,
	const uint8_t *key, size_t len_key,
	const uint8_t *val0, size_t len_val0,
	const uint8_t *val1, size_t len_val1,
	uint8_t **merged_val, size_t *len_merged_val);

typedef void *(*mtbl_merge_init_func)(void);
typedef void (*mtbl_merge_free_func)(void *clos);

/* iter */

typedef mtbl_res (*mtbl_iter_next_func)(void *,
	const uint8_t **key, size_t *len_key,
	const uint8_t **val, size_t *len_val);

typedef void (*mtbl_iter_free_func)(void *);

struct mtbl_iter *mtbl_iter_init(
	mtbl_iter_next_func,
	mtbl_iter_free_func,
	void *clos);
void mtbl_iter_destroy(struct mtbl_iter **);
mtbl_res mtbl_iter_next(struct mtbl_iter *,
	const uint8_t **key, size_t *len_key,
	const uint8_t **val, size_t *len_val)
	__attribute__((warn_unused_result));

/* source */

typedef struct mtbl_iter *(*mtbl_source_iter_func)(void *);

typedef struct mtbl_iter *(*mtbl_source_get_func)(void *,
	const uint8_t *key, size_t len_key);

typedef struct mtbl_iter *(*mtbl_source_get_prefix_func)(void *,
	const uint8_t *key, size_t len_key);

typedef struct mtbl_iter *(*mtbl_source_get_range_func)(void *,
	const uint8_t *key0, size_t len_key0,
	const uint8_t *key1, size_t len_key1);

typedef void (*mtbl_source_free_func)(void *);

struct mtbl_source *mtbl_source_init(
	mtbl_source_iter_func,
	mtbl_source_get_func,
	mtbl_source_get_prefix_func,
	mtbl_source_get_range_func,
	mtbl_source_free_func,
	void *clos);

void mtbl_source_destroy(struct mtbl_source **);

struct mtbl_iter *
mtbl_source_iter(struct mtbl_source *);

struct mtbl_iter *
mtbl_source_get(struct mtbl_source *,
	const uint8_t *key, size_t len_key);

struct mtbl_iter *
mtbl_source_get_prefix(struct mtbl_source *,
	const uint8_t *key, size_t len_key);

struct mtbl_iter *
mtbl_source_get_range(struct mtbl_source *,
	const uint8_t *key0, size_t len_key0,
	const uint8_t *key1, size_t len_key1);

/* writer */

struct mtbl_writer *mtbl_writer_init(
	const char *fname,
	const struct mtbl_writer_options *);
struct mtbl_writer *mtbl_writer_init_fd(
	int fd,
	const struct mtbl_writer_options *);
void mtbl_writer_destroy(struct mtbl_writer **);
mtbl_res mtbl_writer_add(struct mtbl_writer *,
	const uint8_t *key, size_t len_key,
	const uint8_t *val, size_t len_val);

/* writer options */

struct mtbl_writer_options *mtbl_writer_options_init(void);
void mtbl_writer_options_destroy(struct mtbl_writer_options **);
void mtbl_writer_options_set_compression(
	struct mtbl_writer_options *,
	mtbl_compression_type);
void mtbl_writer_options_set_block_size(
	struct mtbl_writer_options *,
	size_t);
void mtbl_writer_options_set_block_restart_interval(
	struct mtbl_writer_options *,
	size_t);

/* reader */

struct mtbl_reader *mtbl_reader_init(
	const char *fname,
	const struct mtbl_reader_options *);
struct mtbl_reader *mtbl_reader_init_fd(
	int fd,
	const struct mtbl_reader_options *);
void mtbl_reader_destroy(struct mtbl_reader **);
mtbl_res mtbl_reader_get(struct mtbl_reader *,
	const uint8_t *key, size_t len_key,
	uint8_t **val, size_t *len_val);
struct mtbl_iter *mtbl_reader_iter(struct mtbl_reader *);
struct mtbl_source *mtbl_reader_source(struct mtbl_reader *);

/* reader options */

struct mtbl_reader_options *mtbl_reader_options_init(void);
void mtbl_reader_options_destroy(struct mtbl_reader_options **);
void mtbl_reader_options_set_verify_checksums(
	struct mtbl_reader_options *,
	bool);

/* merger */

struct mtbl_merger *mtbl_merger_init(const struct mtbl_merger_options *);
void mtbl_merger_destroy(struct mtbl_merger **);
void mtbl_merger_add_reader(
	struct mtbl_merger *,
	struct mtbl_reader *);
mtbl_res mtbl_merger_write(
	struct mtbl_merger *,
	struct mtbl_writer *)
	__attribute__((warn_unused_result));
struct mtbl_iter *mtbl_merger_iter(struct mtbl_merger *);
struct mtbl_iter *mtbl_merger_get(struct mtbl_merger *,
	const uint8_t *key, size_t len_key);
struct mtbl_source *mtbl_merger_source(struct mtbl_merger *);

/* merger options */

struct mtbl_merger_options *mtbl_merger_options_init(void);
void mtbl_merger_options_destroy(struct mtbl_merger_options **);
void mtbl_merger_options_set_merge_func(
	struct mtbl_merger_options *,
	mtbl_merge_func,
	void *clos);

/* sorter */

struct mtbl_sorter *mtbl_sorter_init(struct mtbl_sorter_options *);
void mtbl_sorter_destroy(struct mtbl_sorter **);
mtbl_res mtbl_sorter_add(struct mtbl_sorter *,
	const uint8_t *key, size_t len_key,
	const uint8_t *val, size_t len_val)
	__attribute__((warn_unused_result));
mtbl_res mtbl_sorter_write(
	struct mtbl_sorter *,
	struct mtbl_writer *)
	__attribute__((warn_unused_result));
struct mtbl_iter *mtbl_sorter_iter(struct mtbl_sorter *s);

/* sorter options */

struct mtbl_sorter_options *mtbl_sorter_options_init(void);
void mtbl_sorter_options_destroy(struct mtbl_sorter_options **);
void mtbl_sorter_options_set_merge_func(
	struct mtbl_sorter_options *,
	mtbl_merge_func merge_fp,
	void *clos);
void mtbl_sorter_options_set_temp_dir(
	struct mtbl_sorter_options *,
	const char *);
void mtbl_sorter_options_set_max_memory(
	struct mtbl_sorter_options *,
	size_t);

/* crc32c */

uint32_t mtbl_crc32c(const uint8_t *buffer, size_t length);

/* fixed */

size_t mtbl_fixed_encode32(uint8_t *dst, uint32_t value);
size_t mtbl_fixed_encode64(uint8_t *dst, uint64_t value);
uint32_t mtbl_fixed_decode32(const uint8_t *ptr);
uint64_t mtbl_fixed_decode64(const uint8_t *ptr);

/* varint */

unsigned mtbl_varint_length(uint64_t v);
unsigned mtbl_varint_length_packed(const uint8_t *buf);
size_t mtbl_varint_encode32(uint8_t *ptr, uint32_t value);
size_t mtbl_varint_encode64(uint8_t *ptr, uint64_t value);
size_t mtbl_varint_decode32(const uint8_t *ptr, uint32_t *value);
size_t mtbl_varint_decode64(const uint8_t *ptr, uint64_t *value);

#ifdef __cplusplus
}
#endif

#endif /* MTBL_H */
