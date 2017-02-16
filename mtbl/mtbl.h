/*
 * Copyright (c) 2012-2016 by Farsight Security, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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
	mtbl_res_failure = 0,
	mtbl_res_success = 1
} mtbl_res;

/* compression */

typedef enum {
	MTBL_COMPRESSION_NONE = 0,
	MTBL_COMPRESSION_SNAPPY = 1,
	MTBL_COMPRESSION_ZLIB = 2,
	MTBL_COMPRESSION_LZ4 = 3,
	MTBL_COMPRESSION_LZ4HC = 4,
	MTBL_COMPRESSION_ZSTD = 5,
} mtbl_compression_type;

mtbl_res
mtbl_compress(
	mtbl_compression_type,
	const uint8_t *input,
	const size_t input_size,
	uint8_t **output,
	size_t *output_size);

mtbl_res
mtbl_compress_level(
	mtbl_compression_type,
	int compression_level,
	const uint8_t *input,
	const size_t input_size,
	uint8_t **output,
	size_t *output_size);

mtbl_res
mtbl_decompress(
	mtbl_compression_type,
	const uint8_t *input,
	const size_t input_size,
	uint8_t **output,
	size_t *output_size);

const char *
mtbl_compression_type_to_str(mtbl_compression_type);

mtbl_res
mtbl_compression_type_from_str(const char *, mtbl_compression_type *);

/* exported types */

struct mtbl_iter;
struct mtbl_source;

struct mtbl_reader;
struct mtbl_reader_options;
struct mtbl_writer;
struct mtbl_writer_options;

struct mtbl_metadata;
struct mtbl_merger;
struct mtbl_merger_options;
struct mtbl_fileset;
struct mtbl_fileset_options;
struct mtbl_sorter;
struct mtbl_sorter_options;

typedef void
(*mtbl_merge_func)(void *clos,
	const uint8_t *key, size_t len_key,
	const uint8_t *val0, size_t len_val0,
	const uint8_t *val1, size_t len_val1,
	uint8_t **merged_val, size_t *len_merged_val);

typedef void *
(*mtbl_merge_init_func)(void);

typedef void
(*mtbl_merge_free_func)(void *clos);

typedef bool
(*mtbl_filename_filter_func)(const char *);

/* iter */

typedef mtbl_res
(*mtbl_iter_seek_func)(
	void *,
	const uint8_t *key, size_t len_key);

typedef mtbl_res
(*mtbl_iter_next_func)(
	void *,
	const uint8_t **key, size_t *len_key,
	const uint8_t **val, size_t *len_val);

typedef void
(*mtbl_iter_free_func)(void *);

void
mtbl_iter_destroy(struct mtbl_iter **);

mtbl_res
mtbl_iter_seek(
	struct mtbl_iter *,
	const uint8_t *key, size_t len_key)
__attribute__((warn_unused_result));

mtbl_res
mtbl_iter_next(
	struct mtbl_iter *,
	const uint8_t **key, size_t *len_key,
	const uint8_t **val, size_t *len_val)
__attribute__((warn_unused_result));

/* source */

typedef struct mtbl_iter *
(*mtbl_source_iter_func)(void *);

typedef struct mtbl_iter *
(*mtbl_source_get_func)(void *, const uint8_t *key, size_t len_key);

typedef struct mtbl_iter *
(*mtbl_source_get_prefix_func)(void *, const uint8_t *key, size_t len_key);

typedef struct mtbl_iter *
(*mtbl_source_get_range_func)(
	void *,
	const uint8_t *key0, size_t len_key0,
	const uint8_t *key1, size_t len_key1);

typedef void (*mtbl_source_free_func)(void *);

struct mtbl_source *
mtbl_source_init(
	mtbl_source_iter_func,
	mtbl_source_get_func,
	mtbl_source_get_prefix_func,
	mtbl_source_get_range_func,
	mtbl_source_free_func,
	void *clos);

void
mtbl_source_destroy(struct mtbl_source **);

struct mtbl_iter *
mtbl_source_iter(const struct mtbl_source *);

struct mtbl_iter *
mtbl_source_get(const struct mtbl_source *, const uint8_t *key, size_t len_key);

struct mtbl_iter *
mtbl_source_get_prefix(const struct mtbl_source *, const uint8_t *key, size_t len_key);

struct mtbl_iter *
mtbl_source_get_range(
	const struct mtbl_source *,
	const uint8_t *key0, size_t len_key0,
	const uint8_t *key1, size_t len_key1);

mtbl_res
mtbl_source_write(const struct mtbl_source *, struct mtbl_writer *)
__attribute__((warn_unused_result));

/* writer */

struct mtbl_writer *
mtbl_writer_init(const char *fname, const struct mtbl_writer_options *);

struct mtbl_writer *
mtbl_writer_init_fd(int fd, const struct mtbl_writer_options *);

void
mtbl_writer_destroy(struct mtbl_writer **);

mtbl_res
mtbl_writer_add(
	struct mtbl_writer *,
	const uint8_t *key, size_t len_key,
	const uint8_t *val, size_t len_val)
__attribute__((warn_unused_result));

/* writer options */

struct mtbl_writer_options *
mtbl_writer_options_init(void);

void
mtbl_writer_options_destroy(struct mtbl_writer_options **);

void
mtbl_writer_options_set_compression(
	struct mtbl_writer_options *,
	mtbl_compression_type);

void
mtbl_writer_options_set_compression_level(
	struct mtbl_writer_options *,
	int);

void
mtbl_writer_options_set_block_size(
	struct mtbl_writer_options *,
	size_t);

void
mtbl_writer_options_set_block_restart_interval(
	struct mtbl_writer_options *,
	size_t);

/* reader */

struct mtbl_reader *
mtbl_reader_init(const char *fname, const struct mtbl_reader_options *);

struct mtbl_reader *
mtbl_reader_init_fd(int fd, const struct mtbl_reader_options *);

void
mtbl_reader_destroy(struct mtbl_reader **);

const struct mtbl_source *
mtbl_reader_source(struct mtbl_reader *);

const struct mtbl_metadata *
mtbl_reader_metadata(struct mtbl_reader *);

/* reader options */

struct mtbl_reader_options *
mtbl_reader_options_init(void);

void
mtbl_reader_options_destroy(struct mtbl_reader_options **);

void
mtbl_reader_options_set_madvise_random(struct mtbl_reader_options *, bool);

void
mtbl_reader_options_set_verify_checksums(struct mtbl_reader_options *, bool);

/* metadata */

typedef enum {
	MTBL_FORMAT_V1 = 0,
	MTBL_FORMAT_V2 = 1
} mtbl_file_version;

mtbl_file_version
mtbl_metadata_file_version(const struct mtbl_metadata *);

uint64_t
mtbl_metadata_index_block_offset(const struct mtbl_metadata *);

uint64_t
mtbl_metadata_data_block_size(const struct mtbl_metadata *);

uint64_t
mtbl_metadata_compression_algorithm(const struct mtbl_metadata *);

uint64_t
mtbl_metadata_count_entries(const struct mtbl_metadata *);

uint64_t
mtbl_metadata_count_data_blocks(const struct mtbl_metadata *);

uint64_t
mtbl_metadata_bytes_data_blocks(const struct mtbl_metadata *);

uint64_t
mtbl_metadata_bytes_index_block(const struct mtbl_metadata *);

uint64_t
mtbl_metadata_bytes_keys(const struct mtbl_metadata *);

uint64_t
mtbl_metadata_bytes_values(const struct mtbl_metadata *);

/* merger */

struct mtbl_merger *
mtbl_merger_init(const struct mtbl_merger_options *);

void
mtbl_merger_destroy(struct mtbl_merger **);

void
mtbl_merger_add_source(struct mtbl_merger *, const struct mtbl_source *);

const struct mtbl_source *
mtbl_merger_source(struct mtbl_merger *);

/* merger options */

struct mtbl_merger_options *
mtbl_merger_options_init(void);

void
mtbl_merger_options_destroy(struct mtbl_merger_options **);

void
mtbl_merger_options_set_merge_func(
	struct mtbl_merger_options *,
	mtbl_merge_func,
	void *clos);

/* fileset */

struct mtbl_fileset *
mtbl_fileset_init(const char *fname, const struct mtbl_fileset_options *);

void
mtbl_fileset_destroy(struct mtbl_fileset **);

void
mtbl_fileset_reload(struct mtbl_fileset *);

void
mtbl_fileset_reload_now(struct mtbl_fileset *);

const struct mtbl_source *
mtbl_fileset_source(struct mtbl_fileset *);

void
mtbl_fileset_partition(struct mtbl_fileset *,
		mtbl_filename_filter_func,
		struct mtbl_merger **,
		struct mtbl_merger **);

/* fileset options */

struct mtbl_fileset_options *
mtbl_fileset_options_init(void);

void
mtbl_fileset_options_destroy(struct mtbl_fileset_options **);

void
mtbl_fileset_options_set_merge_func(
	struct mtbl_fileset_options *,
	mtbl_merge_func,
	void *clos);

void
mtbl_fileset_options_set_reload_interval(
	struct mtbl_fileset_options *,
	uint32_t reload_interval);

/* sorter */

struct mtbl_sorter *
mtbl_sorter_init(const struct mtbl_sorter_options *);

void
mtbl_sorter_destroy(struct mtbl_sorter **);

mtbl_res
mtbl_sorter_add(struct mtbl_sorter *,
	const uint8_t *key, size_t len_key,
	const uint8_t *val, size_t len_val)
__attribute__((warn_unused_result));

mtbl_res
mtbl_sorter_write(struct mtbl_sorter *, struct mtbl_writer *)
__attribute__((warn_unused_result));

struct mtbl_iter *
mtbl_sorter_iter(struct mtbl_sorter *s);

/* sorter options */

struct mtbl_sorter_options *
mtbl_sorter_options_init(void);

void
mtbl_sorter_options_destroy(struct mtbl_sorter_options **);

void
mtbl_sorter_options_set_merge_func(
	struct mtbl_sorter_options *,
	mtbl_merge_func merge_fp,
	void *clos);

void
mtbl_sorter_options_set_temp_dir(
	struct mtbl_sorter_options *,
	const char *);

void
mtbl_sorter_options_set_max_memory(
	struct mtbl_sorter_options *,
	size_t);

/* crc32c */

uint32_t
mtbl_crc32c(const uint8_t *buffer, size_t length);

/* fixed */

size_t
mtbl_fixed_encode32(uint8_t *dst, uint32_t value);

size_t
mtbl_fixed_encode64(uint8_t *dst, uint64_t value);

uint32_t
mtbl_fixed_decode32(const uint8_t *ptr);

uint64_t
mtbl_fixed_decode64(const uint8_t *ptr);

/* varint */

unsigned
mtbl_varint_length(uint64_t v);

unsigned
mtbl_varint_length_packed(const uint8_t *buf, size_t len_buf);

size_t
mtbl_varint_encode32(uint8_t *ptr, uint32_t value);

size_t
mtbl_varint_encode64(uint8_t *ptr, uint64_t value);

size_t
mtbl_varint_decode32(const uint8_t *ptr, uint32_t *value);

size_t
mtbl_varint_decode64(const uint8_t *ptr, uint64_t *value);

#ifdef __cplusplus
}
#endif

#endif /* MTBL_H */
