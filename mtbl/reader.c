/*
 * Copyright (c) 2012-2014 by Farsight Security, Inc.
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

#include "mtbl-private.h"

#include "libmy/ubuf.h"

typedef enum {
	READER_ITER_TYPE_ITER,
	READER_ITER_TYPE_GET,
	READER_ITER_TYPE_GET_PREFIX,
	READER_ITER_TYPE_GET_RANGE,
} reader_iter_type;

struct reader_iter {
	struct mtbl_reader		*r;
	struct block			*b;
	struct block_iter		*bi;
	struct block_iter		*index_iter;
	ubuf				*k;
	bool				first;
	bool				valid;
	reader_iter_type		it_type;
};

struct mtbl_reader_options {
	bool				verify_checksums;
	bool				madvise_random;
};

struct mtbl_reader {
	int				fd;
	struct mtbl_metadata		m;
	uint8_t				*data;
	size_t				len_data;
	struct mtbl_reader_options	opt;
	struct block			*index;
	struct mtbl_source		*source;
};

static void
reader_init_madvise(struct mtbl_reader *);

static mtbl_res
reader_iter_next(void *, const uint8_t **, size_t *, const uint8_t **, size_t *);

static void
reader_iter_free(void *);

static struct mtbl_iter *
reader_iter(void *);

static struct mtbl_iter *
reader_get(void *, const uint8_t *, size_t);

static struct mtbl_iter *
reader_get_prefix(void *, const uint8_t *, size_t);

static struct mtbl_iter *
reader_get_range(void *, const uint8_t *, size_t, const uint8_t *, size_t);

struct mtbl_reader_options *
mtbl_reader_options_init(void)
{
	return (my_calloc(1, sizeof(struct mtbl_reader_options)));
}

void
mtbl_reader_options_destroy(struct mtbl_reader_options **opt)
{
	if (*opt) {
		free(*opt);
		*opt = NULL;
	}
}

void
mtbl_reader_options_set_madvise_random(struct mtbl_reader_options *opt,
				       bool madvise_random)
{
	opt->madvise_random = madvise_random;
}

void
mtbl_reader_options_set_verify_checksums(struct mtbl_reader_options *opt,
					 bool verify_checksums)
{
	opt->verify_checksums = verify_checksums;
}

const struct mtbl_metadata *
mtbl_reader_metadata(struct mtbl_reader *r)
{
	return &r->m;
}

static void
reader_init_madvise(struct mtbl_reader *r)
{
	bool b;
	const char *s;

	b = r->opt.madvise_random;
	s = getenv("MTBL_READER_MADVISE_RANDOM");

	if (s) {
		if (strcmp(s, "0") == 0)
			b = false;
		else if (strcmp(s, "1") == 0)
			b = true;
	}

	if (b) {
#if defined(HAVE_POSIX_MADVISE)
		(void) posix_madvise(r->data, r->m.index_block_offset, POSIX_MADV_RANDOM);
#elif defined(HAVE_MADVISE)
		(void) madvise(r->data, r->m.index_block_offset, MADV_RANDOM);
#endif
	}
}

struct mtbl_reader *
mtbl_reader_init_fd(int orig_fd, const struct mtbl_reader_options *opt)
{
	struct mtbl_reader *r;
	struct stat ss;
	size_t metadata_offset;
	int fd;

	size_t index_len;
	uint32_t index_crc;
	uint8_t *index_data;

	assert(orig_fd >= 0);
	fd = dup(orig_fd);
	assert(fd >= 0);
	int ret = fstat(fd, &ss);
	assert(ret == 0);

	if (ss.st_size < MTBL_METADATA_SIZE) {
		close(fd);
		return (NULL);
	}

	r = my_calloc(1, sizeof(*r));
	if (opt != NULL)
		memcpy(&r->opt, opt, sizeof(*opt));
	r->fd = fd;
	r->len_data = ss.st_size;
	r->data = mmap(NULL, r->len_data, PROT_READ, MAP_PRIVATE, r->fd, 0);
	if (r->data == MAP_FAILED) {
		close(r->fd);
		free(r);
		return (NULL);
	}

	metadata_offset = r->len_data - MTBL_METADATA_SIZE;
	if (!metadata_read(r->data + metadata_offset, &r->m)) {
		mtbl_reader_destroy(&r);
		return (NULL);
	}

	/**
	 * Sanitize the index block offset.
	 * We calculate the maximum possible index block offset for this file to
	 * be the total size of the file (r->len_data) minus the length of the
	 * metadata block (MTBL_METADATA_SIZE) minus the length of the minimum
	 * sized block, which requires 4 fixed-length 32-bit integers (16 bytes).
	 */
	const uint64_t max_index_block_offset = r->len_data - MTBL_METADATA_SIZE - 16;
	if (r->m.index_block_offset > max_index_block_offset) {
		mtbl_reader_destroy(&r);
		return (NULL);
	}

	reader_init_madvise(r);

	index_len = mtbl_fixed_decode32(r->data + r->m.index_block_offset + 0);
	index_crc = mtbl_fixed_decode32(r->data + r->m.index_block_offset + sizeof(uint32_t));
	index_data = r->data + r->m.index_block_offset + 2 * sizeof(uint32_t);
	assert(index_crc == mtbl_crc32c(index_data, index_len));
	r->index = block_init(index_data, index_len, false);
	r->source = mtbl_source_init(reader_iter,
				     reader_get,
				     reader_get_prefix,
				     reader_get_range,
				     NULL, r);
	return (r);
}

struct mtbl_reader *
mtbl_reader_init(const char *fname, const struct mtbl_reader_options *opt)
{
	struct mtbl_reader *r;
	int fd;

	fd = open(fname, O_RDONLY);
	if (fd < 0)
		return (NULL);
	r = mtbl_reader_init_fd(fd, opt);
	close(fd);

	return (r);
}

void
mtbl_reader_destroy(struct mtbl_reader **r)
{
	if (*r != NULL) {
		block_destroy(&(*r)->index);
		munmap((*r)->data, (*r)->len_data);
		close((*r)->fd);
		mtbl_source_destroy(&(*r)->source);
		free(*r);
		*r = NULL;
	}
}

const struct mtbl_source *
mtbl_reader_source(struct mtbl_reader *r)
{
	assert(r != NULL);
	return (r->source);
}

static void
get_block_zlib_decompress(uint8_t *raw_contents, size_t raw_contents_size,
			  uint8_t **block_contents, size_t *block_contents_size)
{
	int zret;
	z_stream zs;

	memset(&zs, 0, sizeof(zs));
	zs.zalloc = Z_NULL;
	zs.zfree = Z_NULL;
	zs.opaque = Z_NULL;
	zs.avail_in = 0;
	zs.next_in = Z_NULL;

	zret = inflateInit(&zs);
	assert(zret == Z_OK);
	zs.avail_in = raw_contents_size;
	zs.next_in = raw_contents;
	zs.avail_out = *block_contents_size;
	zs.next_out = *block_contents = my_malloc(*block_contents_size);

	do {
		zret = inflate(&zs, Z_FINISH);
		assert(zret == Z_STREAM_END || zret == Z_BUF_ERROR);
		if (zret != Z_STREAM_END) {
			*block_contents = my_realloc(*block_contents,
						     *block_contents_size * 2);
			zs.next_out = *block_contents + *block_contents_size;
			zs.avail_out = *block_contents_size;
			*block_contents_size *= 2;
		}
	} while (zret != Z_STREAM_END);

	*block_contents_size = zs.total_out;
	inflateEnd(&zs);
}

static struct block *
get_block(struct mtbl_reader *r, uint64_t offset)
{
	bool needs_free = false;
	uint8_t *block_contents = NULL, *raw_contents = NULL;
	size_t block_contents_size = 0, raw_contents_size = 0;
	snappy_status res;

	assert(offset < r->len_data);

	raw_contents_size = mtbl_fixed_decode32(&r->data[offset + 0]);
	raw_contents = &r->data[offset + 2 * sizeof(uint32_t)];

	if (r->opt.verify_checksums) {
		uint32_t block_crc, calc_crc;
		block_crc = mtbl_fixed_decode32(&r->data[offset + sizeof(uint32_t)]);
		calc_crc = mtbl_crc32c(raw_contents, raw_contents_size);
		assert(block_crc == calc_crc);
	}

	switch (r->m.compression_algorithm) {
	case MTBL_COMPRESSION_NONE:
		block_contents = raw_contents;
		block_contents_size = raw_contents_size;
		break;
	case MTBL_COMPRESSION_SNAPPY:
		needs_free = true;
		res = snappy_uncompressed_length((const char *)raw_contents,
						 raw_contents_size,
						 &block_contents_size);
		assert(res == SNAPPY_OK);
		block_contents = my_malloc(block_contents_size);
		res = snappy_uncompress((const char *)raw_contents, raw_contents_size,
					(char *)block_contents, &block_contents_size);
		assert(res == SNAPPY_OK);
		break;
	case MTBL_COMPRESSION_ZLIB:
		needs_free = true;
		block_contents_size = 4 * r->m.data_block_size;
		get_block_zlib_decompress(raw_contents, raw_contents_size,
					  &block_contents, &block_contents_size);
		break;
	}

	return (block_init(block_contents, block_contents_size, needs_free));
}

static struct block *
get_block_at_index(struct mtbl_reader *r, struct block_iter *index_iter)
{
	const uint8_t *ikey, *ival;
	size_t len_ikey, len_ival;

	if (block_iter_get(index_iter, &ikey, &len_ikey, &ival, &len_ival)) {
		struct block *b;
		uint64_t offset;

		mtbl_varint_decode64(ival, &offset);
		b = get_block(r, offset);
		return (b);
	}

	return (NULL);
}

static struct mtbl_iter *
reader_iter(void *clos)
{
	struct mtbl_reader *r = (struct mtbl_reader *) clos;
	struct reader_iter *it = my_calloc(1, sizeof(*it));

	it->r = r;
	it->index_iter = block_iter_init(r->index);

	block_iter_seek_to_first(it->index_iter);
	it->b = get_block_at_index(r, it->index_iter);
	if (it->b == NULL) {
		block_iter_destroy(&it->index_iter);
		block_destroy(&it->b);
		free(it);
		return (NULL);
	}

	it->bi = block_iter_init(it->b);
	block_iter_seek_to_first(it->bi);

	it->first = true;
	it->valid = true;
	it->it_type = READER_ITER_TYPE_ITER;
	return (mtbl_iter_init(reader_iter_next, reader_iter_free, it));
}

static struct reader_iter *
reader_iter_init(struct mtbl_reader *r, const uint8_t *key, size_t len_key)
{
	struct reader_iter *it = my_calloc(1, sizeof(*it));

	it->r = r;
	it->index_iter = block_iter_init(r->index);

	block_iter_seek(it->index_iter, key, len_key);
	it->b = get_block_at_index(r, it->index_iter);
	if (it->b == NULL) {
		block_iter_destroy(&it->index_iter);
		block_destroy(&it->b);
		free(it);
		return (NULL);
	}

	it->bi = block_iter_init(it->b);
	block_iter_seek(it->bi, key, len_key);

	it->first = true;
	it->valid = true;
	return (it);
}

static struct mtbl_iter *
reader_get(void *clos, const uint8_t *key, size_t len_key)
{
	struct mtbl_reader *r = (struct mtbl_reader *) clos;
	struct reader_iter *it = reader_iter_init(r, key, len_key);
	if (it == NULL)
		return (NULL);
	it->k = ubuf_init(len_key);
	ubuf_append(it->k, key, len_key);
	it->it_type = READER_ITER_TYPE_GET;
	return (mtbl_iter_init(reader_iter_next, reader_iter_free, it));
}

static struct mtbl_iter *
reader_get_prefix(void *clos, const uint8_t *key, size_t len_key)
{
	struct mtbl_reader *r = (struct mtbl_reader *) clos;
	struct reader_iter *it = reader_iter_init(r, key, len_key);
	if (it == NULL)
		return (NULL);
	it->k = ubuf_init(len_key);
	ubuf_append(it->k, key, len_key);
	it->it_type = READER_ITER_TYPE_GET_PREFIX;
	return (mtbl_iter_init(reader_iter_next, reader_iter_free, it));
}

static struct mtbl_iter *
reader_get_range(void *clos,
		 const uint8_t *key0, size_t len_key0,
		 const uint8_t *key1, size_t len_key1)
{
	struct mtbl_reader *r = (struct mtbl_reader *) clos;
	struct reader_iter *it = reader_iter_init(r, key0, len_key0);
	if (it == NULL)
		return (NULL);
	it->k = ubuf_init(len_key1);
	ubuf_append(it->k, key1, len_key1);
	it->it_type = READER_ITER_TYPE_GET_RANGE;
	return (mtbl_iter_init(reader_iter_next, reader_iter_free, it));
}

static void
reader_iter_free(void *v)
{
	struct reader_iter *it = (struct reader_iter *) v;
	if (it) {
		ubuf_destroy(&it->k);
		block_destroy(&it->b);
		block_iter_destroy(&it->bi);
		block_iter_destroy(&it->index_iter);
		free(it);
	}
}

static mtbl_res
reader_iter_next(void *v,
	       const uint8_t **key, size_t *len_key,
	       const uint8_t **val, size_t *len_val)
{
	struct reader_iter *it = (struct reader_iter *) v;
	if (!it->valid)
		return (mtbl_res_failure);

	if (!it->first)
		block_iter_next(it->bi);
	it->first = false;

	it->valid = block_iter_get(it->bi, key, len_key, val, len_val);
	if (!it->valid) {
		block_destroy(&it->b);
		block_iter_destroy(&it->bi);
		if (!block_iter_next(it->index_iter))
			return (mtbl_res_failure);
		it->b = get_block_at_index(it->r, it->index_iter);
		it->bi = block_iter_init(it->b);
		block_iter_seek_to_first(it->bi);
		it->valid = block_iter_get(it->bi, key, len_key, val, len_val);
		if (!it->valid)
			return (mtbl_res_failure);
	}

	switch (it->it_type) {
	case READER_ITER_TYPE_ITER:
		break;
	case READER_ITER_TYPE_GET:
		if (bytes_compare(*key, *len_key, ubuf_data(it->k), ubuf_size(it->k)) != 0)
			it->valid = false;
		break;
	case READER_ITER_TYPE_GET_PREFIX:
		if (!(ubuf_size(it->k) <= *len_key &&
		      memcmp(ubuf_data(it->k), *key, ubuf_size(it->k)) == 0))
		{
			it->valid = false;
		}
		break;
	case READER_ITER_TYPE_GET_RANGE:
		if (bytes_compare(*key, *len_key, ubuf_data(it->k), ubuf_size(it->k)) > 0)
			it->valid = false;
		break;
	default:
		assert(0);
	}

	if (it->valid)
		return (mtbl_res_success);
	return (mtbl_res_failure);
}
