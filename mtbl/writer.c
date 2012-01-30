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

struct mtbl_writer {
	char				*fname;
	int				fd;
	struct mtbl_trailer		t;
	struct mtbl_block_builder	*data;
	struct mtbl_block_builder	*index;

	ubuf				*last_key;
	uint64_t			last_offset;

	size_t				block_size;
	bool				closed;
	bool				pending_index_entry;
	uint64_t			pending_offset;
};

static void _mtbl_writer_finish(struct mtbl_writer *);
static void _mtbl_writer_flush(struct mtbl_writer *);
static void _write_all(int fd, const uint8_t *, size_t);
static size_t _mtbl_writer_writeblock(struct mtbl_writer *, struct mtbl_block_builder *);

struct mtbl_writer *
mtbl_writer_init(const char *fname)
{
	struct mtbl_writer *w;
	int fd;

	fd = open(fname, O_WRONLY | O_CREAT | O_EXCL, 0644);
	if (fd < 0)
		return (NULL);
	w = calloc(1, sizeof(*w));
	assert(w != NULL);
	w->fd = fd;
	w->fname = strdup(fname);
	w->data = mtbl_block_builder_init();
	w->index = mtbl_block_builder_init();
	w->last_key = ubuf_init(256);
	w->block_size = 16384;
	w->t.compression_algorithm = MTBL_COMP_NONE;

	return (w);
}

void
mtbl_writer_destroy(struct mtbl_writer **w)
{
	if (!(*w)->closed) {
		_mtbl_writer_finish(*w);
		close((*w)->fd);
	}
	/* XXX */
	if (*w != NULL) {
		mtbl_block_builder_destroy(&((*w)->data));
		mtbl_block_builder_destroy(&((*w)->index));
		free((*w)->fname);
		free(*w);
		*w = NULL;
	}
}

void
mtbl_writer_add(struct mtbl_writer *w,
		const uint8_t *key, size_t len_key,
		const uint8_t *val, size_t len_val)
{
	assert(!w->closed);
	if (w->t.count_entries > 0) {
		/* XXX use comparator to ensure ordering */
	}

	if (w->pending_index_entry) {
		/* XXX use shortest separator */
		uint8_t enc[10];
		size_t len_enc;
		assert(mtbl_block_builder_empty(w->data));
		len_enc = mtbl_varint_encode64(enc, w->pending_offset);
		/*
		fprintf(stderr, "%s: writing index entry, key= '%s' (%zd) val= %" PRIu64 "\n",
			__func__,
			w->last_key->_v, ubuf_size(w->last_key), w->pending_offset);
		*/
		mtbl_block_builder_add(w->index,
				       ubuf_ptr(w->last_key), ubuf_size(w->last_key),
				       enc, len_enc);
		w->pending_index_entry = false;
	}

	ubuf_reset(w->last_key);
	ubuf_append(w->last_key, key, len_key);

	w->t.count_entries += 1;
	w->t.bytes_keys += len_key;
	w->t.bytes_values += len_val;
	mtbl_block_builder_add(w->data, key, len_key, val, len_val);

	const size_t estimated_block_size = mtbl_block_builder_current_size_estimate(w->data);
	if (estimated_block_size >= w->block_size)
		_mtbl_writer_flush(w);
}

static void
_mtbl_writer_finish(struct mtbl_writer *w)
{
	uint8_t tbuf[MTBL_TRAILER_SIZE];

	_mtbl_writer_flush(w);
	assert(!w->closed);
	w->closed = true;
	if (w->pending_index_entry) {
		/* XXX use short successor */
		uint8_t enc[10];
		size_t len_enc;
		len_enc = mtbl_varint_encode64(enc, w->pending_offset);
		/*
		fprintf(stderr, "%s: writing index entry, key= '%s' (%zd) val= %" PRIu64 "\n",
			__func__,
			w->last_key->_v, ubuf_size(w->last_key), w->pending_offset);
		*/
		mtbl_block_builder_add(w->index,
				       ubuf_ptr(w->last_key), ubuf_size(w->last_key),
				       enc, len_enc);
		w->pending_index_entry = false;
	}
	w->t.index_block_offset = w->pending_offset;
	w->t.bytes_index_block = _mtbl_writer_writeblock(w, w->index);

	trailer_write(&w->t, tbuf);
	_write_all(w->fd, tbuf, sizeof(tbuf));
}

static void
_mtbl_writer_flush(struct mtbl_writer *w)
{
	assert(!w->closed);
	if (mtbl_block_builder_empty(w->data))
		return;
	assert(!w->pending_index_entry);
	_mtbl_writer_writeblock(w, w->data);
	w->pending_index_entry = true;
}

static size_t
_mtbl_writer_writeblock(struct mtbl_writer *w, struct mtbl_block_builder *b)
{
	uint8_t *raw_contents = NULL, *block_contents = NULL, *comp_contents = NULL;
	size_t raw_contents_size = 0, block_contents_size = 0, comp_contents_size = 0;
	snappy_status res;

	mtbl_block_builder_finish(b, &raw_contents, &raw_contents_size);
	assert(raw_contents != NULL);

	switch (w->t.compression_algorithm) {
	case MTBL_COMP_NONE:
		block_contents = raw_contents;
		block_contents_size = raw_contents_size;
		w->t.bytes_data_blocks_uncompressed += raw_contents_size;
		break;
	case MTBL_COMP_SNAPPY:
		comp_contents_size = snappy_max_compressed_length(raw_contents_size);
		comp_contents = malloc(comp_contents_size);
		assert(comp_contents != NULL);
		res = snappy_compress((const char *) raw_contents, raw_contents_size,
				      (char *) comp_contents, &comp_contents_size);
		assert(res == SNAPPY_OK);
		block_contents = comp_contents;
		block_contents_size = comp_contents_size;
		w->t.bytes_data_blocks_compressed += comp_contents_size;
		break;
	}

	assert(block_contents_size < UINT_MAX);

	w->t.count_data_blocks += 1;

	const uint32_t crc = htole32(mtbl_crc32c(block_contents, block_contents_size));
	const uint32_t len = htole32(block_contents_size);

	_write_all(w->fd, (const uint8_t *) &len, sizeof(len));
	_write_all(w->fd, (const uint8_t *) &crc, sizeof(crc));
	_write_all(w->fd, block_contents, block_contents_size);

	const size_t bytes_written = (sizeof(len) + sizeof(crc) + block_contents_size);
	w->pending_offset += bytes_written;

	mtbl_block_builder_reset(b);
	free(raw_contents);
	free(comp_contents);

	return (bytes_written);
}

static void
_write_all(int fd, const uint8_t *buf, size_t size)
{
	assert(size > 0);

	while (size) {
		ssize_t bytes_written;

		bytes_written = write(fd, buf, size);
		if (bytes_written < 0 && errno == EINTR)
			continue;
		if (bytes_written <= 0) {
			fprintf(stderr, "%s: write() failed: %s\n", __func__,
				strerror(errno));
			assert(bytes_written > 0);
		}
		buf += bytes_written;
		size -= bytes_written;
	}
}
