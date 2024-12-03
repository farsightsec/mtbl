/*
 * Copyright (c) 2012, 2014-2016 by Farsight Security, Inc.
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
#include "bytes.h"
#include "threadpool.h"
#include "libmy/ubuf.h"
#include "libmy/my_alloc.h"


struct mtbl_writer_options {
	mtbl_compression_type		compression_type;
	int				compression_level;
	size_t				block_size;
	size_t				block_restart_interval;
	struct mtbl_threadpool		*pool;
};

struct mtbl_writer {
	int				fd;
	struct mtbl_metadata		m;
	struct block_builder		*data;
	struct block_builder		*index;

	struct mtbl_writer_options	opt;

	struct threadpool		*pool;
	struct result_handler		*rhandler;

	ubuf				*last_key;
	uint64_t			last_offset;

	bool				closed;
	uint64_t			pending_offset;
};

struct data_block {
	mtbl_compression_type		comp_type;
	int				comp_level;

	uint8_t				*data;
	size_t				len_data;
	uint8_t				*last_key;
	size_t				len_last_key;

	uint32_t			crc;
};


static void _mtbl_writer_finish(struct mtbl_writer *);
static void _mtbl_writer_flush(struct mtbl_writer *);
static void _mtbl_writer_compress_block(struct data_block *);
static size_t _mtbl_writer_write_block(int, struct data_block *);
static void _mtbl_writer_write_data_block(struct mtbl_writer *, struct data_block *);
static void _write_all(int, const uint8_t *, size_t);

static void *_compress_block_wrapper(void *);
static void _write_data_block_wrapper(void *, void *);


struct mtbl_writer_options *
mtbl_writer_options_init(void)
{
	struct mtbl_writer_options *opt;
	opt = my_calloc(1, sizeof(*opt));
	opt->compression_type = DEFAULT_COMPRESSION_TYPE;
	opt->compression_level = DEFAULT_COMPRESSION_LEVEL;
	opt->block_size = DEFAULT_BLOCK_SIZE;
	opt->block_restart_interval = DEFAULT_BLOCK_RESTART_INTERVAL;
	opt->pool = NULL;
	return (opt);
}

void
mtbl_writer_options_destroy(struct mtbl_writer_options **opt)
{
	if (*opt)
		my_free(*opt);
}

void
mtbl_writer_options_set_compression(struct mtbl_writer_options *opt,
				    mtbl_compression_type compression_type)
{
	switch (compression_type) {
	case MTBL_COMPRESSION_NONE:
	case MTBL_COMPRESSION_SNAPPY:
	case MTBL_COMPRESSION_ZLIB:
	case MTBL_COMPRESSION_LZ4:
	case MTBL_COMPRESSION_LZ4HC:
	case MTBL_COMPRESSION_ZSTD:
		break;
	default:
		assert(0);
	}
	opt->compression_type = compression_type;
}

void
mtbl_writer_options_set_compression_level(struct mtbl_writer_options *opt,
					  int compression_level)
{
	opt->compression_level = compression_level;
}

void
mtbl_writer_options_set_block_size(struct mtbl_writer_options *opt,
				   size_t block_size)
{
	if (block_size < MIN_BLOCK_SIZE)
		block_size = MIN_BLOCK_SIZE;
	opt->block_size = block_size;
}

void
mtbl_writer_options_set_block_restart_interval(struct mtbl_writer_options *opt,
					       size_t block_restart_interval)
{
	opt->block_restart_interval = block_restart_interval;
}

void
mtbl_writer_options_set_threadpool(struct mtbl_writer_options *opt,
				   struct mtbl_threadpool *pool)
{
	opt->pool = pool;
}

struct mtbl_writer *
mtbl_writer_init_fd(int orig_fd, const struct mtbl_writer_options *opt)
{
	struct mtbl_writer *w;
	int fd;

	fd = dup(orig_fd);
	assert(fd >= 0);
	w = my_calloc(1, sizeof(*w));
	if (opt == NULL) {
		w->opt.compression_type = DEFAULT_COMPRESSION_TYPE;
		w->opt.compression_level = DEFAULT_COMPRESSION_LEVEL;
		w->opt.block_size = DEFAULT_BLOCK_SIZE;
		w->opt.block_restart_interval = DEFAULT_BLOCK_RESTART_INTERVAL;
		w->opt.pool = NULL;
	} else {
		memcpy(&w->opt, opt, sizeof(*opt));
	}
	w->fd = fd;
	/*
	 * Start writing from the current offset. This allows mtbl's callers
	 * to reserve some initial bytes in the file.
	 */
	w->last_offset = lseek(fd, 0, SEEK_CUR);
	w->pending_offset = w->last_offset;
	w->last_key = ubuf_init(256);
	w->m.file_version = MTBL_FORMAT_V2;
	w->m.compression_algorithm = w->opt.compression_type;
	w->m.data_block_size = w->opt.block_size;
	w->data = block_builder_init(w->opt.block_restart_interval);
	w->index = block_builder_init(w->opt.block_restart_interval);

	/* Initialize result handler to receive completed threadpool work. */
	if (w->opt.pool != NULL) {
		w->pool = w->opt.pool->pool;
		w->rhandler = result_handler_init(_write_data_block_wrapper, w);
	}

	return (w);
}

struct mtbl_writer *
mtbl_writer_init(const char *fname, const struct mtbl_writer_options *opt)
{
	struct mtbl_writer *w;
	int fd;

	fd = open(fname, O_WRONLY | O_CREAT | O_TRUNC | O_EXCL, 0644);
	if (fd < 0)
		return (NULL);
	w = mtbl_writer_init_fd(fd, opt);
	close(fd);
	return (w);
}

void
mtbl_writer_destroy(struct mtbl_writer **w)
{
	if (*w) {
		if (!(*w)->closed) {
			_mtbl_writer_finish(*w);
			close((*w)->fd);
		}

		block_builder_destroy(&((*w)->data));
		block_builder_destroy(&((*w)->index));
		ubuf_destroy(&(*w)->last_key);

		my_free(*w);
	}
}

mtbl_res
mtbl_writer_add(struct mtbl_writer *w,
		const uint8_t *key, size_t len_key,
		const uint8_t *val, size_t len_val)
{
	assert(!w->closed);
	if (w->m.count_entries > 0) {
		if (!(bytes_compare(key, len_key,
				    ubuf_data(w->last_key), ubuf_size(w->last_key)) > 0))
		{
			return (mtbl_res_failure);
		}
	}

	size_t estimated_block_size = block_builder_current_size_estimate(w->data);
	estimated_block_size += 3*5 + len_key + len_val;

	if (estimated_block_size >= w->opt.block_size) {
		bytes_shortest_separator(w->last_key, key, len_key);
		_mtbl_writer_flush(w);
	}

	ubuf_reset(w->last_key);
	ubuf_append(w->last_key, key, len_key);

	w->m.count_entries += 1;
	w->m.bytes_keys += len_key;
	w->m.bytes_values += len_val;
	block_builder_add(w->data, key, len_key, val, len_val);

	return (mtbl_res_success);
}

static void
_mtbl_writer_finish(struct mtbl_writer *w)
{
	struct data_block index;
	uint8_t tbuf[MTBL_METADATA_SIZE];
	size_t bytes_written;

	_mtbl_writer_flush(w);

	result_handler_destroy(&w->rhandler);
	assert(!w->closed);
	w->closed = true;

	/* Write the uncompressed index block to disk. */
	block_builder_finish(w->index, &index.data, &index.len_data);
	index.crc = htole32(mtbl_crc32c(index.data, index.len_data));
	bytes_written = _mtbl_writer_write_block(w->fd, &index);

	/* Write index block metadata. */
	w->m.index_block_offset = w->pending_offset;
	w->m.bytes_index_block = bytes_written;
	w->last_offset = w->pending_offset;
	w->pending_offset += bytes_written;
	metadata_write(&w->m, tbuf);
	_write_all(w->fd, tbuf, sizeof(tbuf));
	block_builder_reset(w->index);
	free(index.data);
}

static void
_mtbl_writer_flush(struct mtbl_writer *w)
{
	struct data_block b;

	assert(!w->closed);
	assert(w->m.file_version == MTBL_FORMAT_V2);

	if (block_builder_empty(w->data))
		return;

	/* Make a copy of our block, in case we send it to the threadpool. */
	b.comp_type = w->opt.compression_type;
	b.comp_level = w->opt.compression_level;
	b.len_last_key = ubuf_size(w->last_key);
	b.last_key = my_malloc(b.len_last_key);
	memcpy(b.last_key, ubuf_data(w->last_key), b.len_last_key);
	block_builder_finish(w->data, &b.data, &b.len_data);
	block_builder_reset(w->data);

	if (w->pool != NULL) {
		struct data_block *bthread = my_calloc(1, sizeof(*bthread));

		memcpy(bthread, &b, sizeof(b));
		threadpool_dispatch(w->pool, w->rhandler, true,	/* ordered */
				    _compress_block_wrapper, (void *)bthread);
	} else {
		_mtbl_writer_compress_block(&b);
		_mtbl_writer_write_data_block(w, &b);
	}
}

static void
_mtbl_writer_compress_block(struct data_block *b)
{
	mtbl_res res;
	struct data_block tmp;

	if (b->comp_type == MTBL_COMPRESSION_NONE) {
		res = mtbl_res_success;
	} else if (b->comp_level == DEFAULT_COMPRESSION_LEVEL) {
		res = mtbl_compress(b->comp_type, b->data, b->len_data,
			&tmp.data, &tmp.len_data);
	} else {
		res = mtbl_compress_level(b->comp_type, b->comp_level,
			b->data, b->len_data, &tmp.data, &tmp.len_data);
	}
	assert(res == mtbl_res_success);

	if (b->comp_type != MTBL_COMPRESSION_NONE) {
		free(b->data);
		b->data = tmp.data;
		b->len_data = tmp.len_data;
	}

	b->crc = htole32(mtbl_crc32c(b->data, b->len_data));
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

static size_t
_mtbl_writer_write_block(int fd, struct data_block *b)
{
	uint8_t len[10];
	size_t len_length, bytes_written;

	len_length = mtbl_varint_encode64(len, b->len_data);

	_write_all(fd, (const uint8_t *) len, len_length);
	_write_all(fd, (const uint8_t *) &b->crc, sizeof(b->crc));
	_write_all(fd, b->data, b->len_data);

	bytes_written = len_length + sizeof(b->crc) + b->len_data;
	return (bytes_written);
}

static void
_mtbl_writer_write_data_block(struct mtbl_writer *w, struct data_block *b)
{
	uint8_t enc[10];
	size_t len_enc, bytes_written;

	bytes_written = _mtbl_writer_write_block(w->fd, b);

	/* Update the writer's metadata with this data block. */
	w->last_offset = w->pending_offset;
	w->pending_offset += bytes_written;
	w->m.bytes_data_blocks += bytes_written;
	w->m.count_data_blocks += 1;

	/* Add an index entry for this data block */
	len_enc = mtbl_varint_encode64(enc, w->last_offset);
	block_builder_add(w->index, b->last_key, b->len_last_key, enc, len_enc);

	free(b->last_key);
	free(b->data);
}

static void *
_compress_block_wrapper(void *block)
{
	if (block == NULL)
		return NULL;

	_mtbl_writer_compress_block(block);
	return block;
}

static void
_write_data_block_wrapper(void *block, void *writer)
{
	if (block == NULL)
		return;

	_mtbl_writer_write_data_block(writer, block);
	free(block);
}
