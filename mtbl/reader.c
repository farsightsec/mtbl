/*
 * Copyright (c) 2012 by Internet Systems Consortium, Inc. ("ISC")
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS.	IN NO EVENT SHALL ISC BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "mtbl-private.h"
#include "vector_types.h"

typedef enum {
	ITER_TYPE_ALL,
	ITER_TYPE_RANGE,
	ITER_TYPE_PREFIX
} iter_type;

struct mtbl_iter {
	struct mtbl_reader		*r;
	struct block			*b;
	struct block_iter		*bi;
	struct block_iter		*index_iter;
	ubuf				*k;
	bool				first;
	bool				valid;
	iter_type			it_type;
};

struct mtbl_reader_options {
	mtbl_compare_fp			compare;
	bool				verify_checksums;
};

struct mtbl_reader {
	char				*fname;
	int				fd;
	struct trailer			t;
	uint8_t				*data;
	size_t				len_data;

	struct mtbl_reader_options	opt;

	struct block			*index;
	struct block_iter		*index_iter;
};

struct mtbl_reader_options *
mtbl_reader_options_init(void)
{
	struct mtbl_reader_options *opt;
	opt = calloc(1, sizeof(*opt));
	assert(opt != NULL);
	opt->compare = DEFAULT_COMPARE_FUNC;
	return (opt);
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
mtbl_reader_options_set_compare(struct mtbl_reader_options *opt,
				mtbl_compare_fp compare)
{
	opt->compare = compare;
}

void
mtbl_reader_options_set_verify_checksums(struct mtbl_reader_options *opt,
					 bool verify_checksums)
{
	opt->verify_checksums = verify_checksums;
}

struct mtbl_reader *
mtbl_reader_init(const char *fname, const struct mtbl_reader_options *opt)
{
	struct mtbl_reader *r;
	struct stat ss;
	size_t trailer_offset;
	int fd;

	size_t index_len;
	uint32_t index_crc;
	uint8_t *index_data;

	fd = open(fname, O_RDONLY);
	if (fd < 0)
		return (NULL);
	if (fstat(fd, &ss) < 0)
		return (NULL);

	r = calloc(1, sizeof(*r));
	assert(r != NULL);
	if (opt == NULL) {
		r->opt.compare = DEFAULT_COMPARE_FUNC;
	} else {
		memcpy(&r->opt, opt, sizeof(*opt));
	}
	r->fd = fd;
	r->fname = strdup(fname);
	r->len_data = ss.st_size;
	r->data = mmap(NULL, r->len_data, PROT_READ, MAP_PRIVATE, r->fd, 0);
	if (r->data == MAP_FAILED) {
		close(r->fd);
		free(r->fname);
		free(r);
		return (NULL);
	}

	trailer_offset = r->len_data - MTBL_TRAILER_SIZE;
	if (!trailer_read(r->data + trailer_offset, &r->t)) {
		mtbl_reader_destroy(&r);
		return (NULL);
	}

	index_len = mtbl_fixed_decode32(r->data + r->t.index_block_offset + 0);
	index_crc = mtbl_fixed_decode32(r->data + r->t.index_block_offset + sizeof(uint32_t));
	index_data = r->data + r->t.index_block_offset + 2 * sizeof(uint32_t);
	assert(index_crc == mtbl_crc32c(index_data, index_len));
	r->index = block_init(index_data, index_len, false, r->opt.compare);
	assert(r->index != NULL);
	r->index_iter = block_iter_init(r->index);
	assert(r->index_iter != NULL);

	return (r);
}

void
mtbl_reader_destroy(struct mtbl_reader **r)
{
	if (*r != NULL) {
		block_iter_destroy(&(*r)->index_iter);
		block_destroy(&(*r)->index);
		munmap((*r)->data, (*r)->len_data);
		close((*r)->fd);
		free((*r)->fname);
		free(*r);
		*r = NULL;
	}
}

static struct block *
get_block(struct mtbl_reader *r, uint64_t offset)
{
	bool needs_free = false;
	struct block *b;
	uint8_t *block_contents = NULL, *raw_contents = NULL;
	size_t block_contents_size = 0, raw_contents_size = 0;
	snappy_status res;
	int zret;
	z_stream zs;

	assert(offset < r->len_data);

	raw_contents_size = mtbl_fixed_decode32(&r->data[offset + 0]);
	raw_contents = &r->data[offset + 2 * sizeof(uint32_t)];

	if (r->opt.verify_checksums) {
		uint32_t block_crc, calc_crc;
		block_crc = mtbl_fixed_decode32(&r->data[offset + sizeof(uint32_t)]);
		calc_crc = mtbl_crc32c(raw_contents, raw_contents_size);
		assert(block_crc == calc_crc);
	}

	switch (r->t.compression_algorithm) {
	case MTBL_COMPRESSION_NONE:
		block_contents = raw_contents;
		block_contents_size = raw_contents_size;
		break;
	case MTBL_COMPRESSION_SNAPPY:
		needs_free = true;
		block_contents_size = 2 * r->t.data_block_size;
		block_contents = calloc(1, block_contents_size);
		assert(block_contents != NULL);
		res = snappy_uncompress((const char *)raw_contents, raw_contents_size,
					(char *) block_contents, &block_contents_size);
		assert(res == SNAPPY_OK);
		break;
	case MTBL_COMPRESSION_ZLIB:
		needs_free = true;
		block_contents_size = 2 * r->t.data_block_size;
		zs.zalloc = Z_NULL;
		zs.zfree = Z_NULL;
		zs.opaque = Z_NULL;
		zs.avail_in = 0;
		zs.next_in = Z_NULL;
		memset(&zs, 0, sizeof(zs));
		zret = inflateInit(&zs);
		assert(zret == Z_OK);
		zs.avail_in = raw_contents_size;
		zs.next_in = raw_contents;
		zs.avail_out = 131072;
		zs.next_out = block_contents = calloc(1, block_contents_size);
		assert(zs.next_out != NULL);
		zret = inflate(&zs, Z_NO_FLUSH);
		assert(zret == Z_STREAM_END);
		block_contents_size = zs.total_out;
		inflateEnd(&zs);
		break;
	}

	b = block_init(block_contents, block_contents_size, needs_free, r->opt.compare);
	assert(b != NULL);

	return (b);
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

bool
mtbl_reader_get(struct mtbl_reader *r,
		const uint8_t *key, size_t len_key,
		uint8_t **val, size_t *len_val)
{
	struct block *b;
	bool res = false;
	const uint8_t *bkey, *bval;
	size_t bkey_len, bval_len;

	block_iter_seek(r->index_iter, key, len_key);
	b = get_block_at_index(r, r->index_iter);
	if (b != NULL) {
		struct block_iter *bi = block_iter_init(b);
		assert(bi != NULL);

		block_iter_seek(bi, key, len_key);
		block_iter_get(bi, &bkey, &bkey_len, &bval, &bval_len);
		if (r->opt.compare(key, len_key, bkey, bkey_len) == 0) {
			*len_val = bval_len;
			*val = malloc(bval_len);
			assert(*val != NULL);
			memcpy(*val, bval, bval_len);
			res = true;
		}
		block_iter_destroy(&bi);
		block_destroy(&b);
	}

	return (res);
}

static struct mtbl_iter *
iter_init(struct mtbl_reader *r, const uint8_t *key, size_t len_key)
{
	struct mtbl_iter *it;
	it = calloc(1, sizeof(*it));
	assert(it != NULL);

	it->r = r;
	it->index_iter = block_iter_init(r->index);
	assert(it->index_iter != NULL);

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

struct mtbl_iter *
mtbl_reader_iter(struct mtbl_reader *r)
{
	struct mtbl_iter *it;
	it = calloc(1, sizeof(*it));
	assert(it != NULL);

	it->r = r;
	it->index_iter = block_iter_init(r->index);
	assert(it->index_iter != NULL);

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
	it->it_type = ITER_TYPE_ALL;
	return (it);
}

struct mtbl_iter *
mtbl_reader_get_range(struct mtbl_reader *r,
		      const uint8_t *key0, size_t len_key0,
		      const uint8_t *key1, size_t len_key1)
{
	struct mtbl_iter *it = iter_init(r, key0, len_key0);
	if (it == NULL)
		return (NULL);
	it->k = ubuf_init(len_key1);
	ubuf_append(it->k, key1, len_key1);
	it->it_type = ITER_TYPE_RANGE;
	return (it);
}

struct mtbl_iter *
mtbl_reader_get_prefix(struct mtbl_reader *r,
		       const uint8_t *key, size_t len_key)
{
	struct mtbl_iter *it = iter_init(r, key, len_key);
	if (it == NULL)
		return (NULL);
	it->k = ubuf_init(len_key);
	ubuf_append(it->k, key, len_key);
	it->it_type = ITER_TYPE_PREFIX;
	return (it);
}

void
mtbl_iter_destroy(struct mtbl_iter **it)
{
	if (*it) {
		ubuf_destroy(&(*it)->k);
		block_destroy(&(*it)->b);
		block_iter_destroy(&(*it)->bi);
		block_iter_destroy(&(*it)->index_iter);
		free(*it);
		*it = NULL;
	}
}

bool
mtbl_iter_next(struct mtbl_iter *it,
	       const uint8_t **key, size_t *len_key,
	       const uint8_t **val, size_t *len_val)
{
	if (!it->valid)
		return (false);

	if (!it->first)
		block_iter_next(it->bi);
	it->first = false;

	it->valid = block_iter_get(it->bi, key, len_key, val, len_val);
	if (!it->valid) {
		block_destroy(&it->b);
		block_iter_destroy(&it->bi);
		if (!block_iter_next(it->index_iter))
			return (false);
		it->b = get_block_at_index(it->r, it->index_iter);
		it->bi = block_iter_init(it->b);
		block_iter_seek_to_first(it->bi);
		it->valid = block_iter_get(it->bi, key, len_key, val, len_val);
		if (!it->valid)
			return (false);
	}

	switch (it->it_type) {
	case ITER_TYPE_ALL:
		break;
	case ITER_TYPE_RANGE:
		if (it->r->opt.compare(*key, *len_key,
				       ubuf_data(it->k), ubuf_size(it->k)) > 0)
		{
			it->valid = false;
		}
		break;
	case ITER_TYPE_PREFIX:
		if (!(ubuf_size(it->k) <= *len_key &&
		      memcmp(ubuf_data(it->k), *key, ubuf_size(it->k)) == 0))
		{
			it->valid = false;
		}
		break;
	default:
		assert(0);
	}

	return (it->valid);
}
