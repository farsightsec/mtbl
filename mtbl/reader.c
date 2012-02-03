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

struct mtbl_reader_options {
	mtbl_compare_fp			compare;
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
	uint32_t crc;
	uint8_t *block_contents = NULL, *raw_contents = NULL;
	size_t block_contents_size = 0, raw_contents_size = 0;
	snappy_status res;
	int zret;
	z_stream zs;

	assert(offset < r->len_data);

	raw_contents_size = mtbl_fixed_decode32(&r->data[offset + 0]);
	crc = mtbl_fixed_decode32(&r->data[offset + sizeof(uint32_t)]);
	raw_contents = &r->data[offset + 2 * sizeof(uint32_t)];

	//assert(crc == mtbl_crc32c(raw_contents, raw_contents_size));

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

bool
mtbl_reader_get(struct mtbl_reader *r,
		const uint8_t *key, size_t key_len,
		uint8_t **val, size_t *val_len)
{
	bool res = false;
	const uint8_t *ikey, *ival, *bkey, *bval;
	size_t ikey_len, ival_len, bkey_len, bval_len;

	block_iter_seek(r->index_iter, key, key_len);
	if (block_iter_get(r->index_iter, &ikey, &ikey_len, &ival, &ival_len)) {
		struct block *b;
		struct block_iter *bi;
		uint64_t offset;

		mtbl_varint_decode64(ival, &offset);
		b = get_block(r, offset);
		bi = block_iter_init(b);
		assert(bi != NULL);

		block_iter_seek(bi, key, key_len);
		block_iter_get(bi, &bkey, &bkey_len, &bval, &bval_len);
		if (r->opt.compare(key, key_len, bkey, bkey_len) == 0) {
			*val_len = bval_len;
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
