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

struct mtbl_block_builder {
	size_t		block_restart_interval;
	size_t		block_size;

	ubuf		*buf;
	uint32_vec	*restarts;

	uint8_t		*last_key;
	size_t		len_last_key;

	bool		finished;
	size_t		counter;
};

struct mtbl_block_builder *
mtbl_block_builder_init(void)
{
	struct mtbl_block_builder *b;

	b = calloc(1, sizeof(*b));
	if (b == NULL)
		return (NULL);

	b->block_restart_interval = 16;
	b->block_size = 16384;

	b->buf = ubuf_init(16384);

	b->restarts = uint32_vec_init(64);
	uint32_vec_add(b->restarts, 0);

	return (b);
}

void
mtbl_block_builder_destroy(struct mtbl_block_builder **b)
{
	if (*b) {
		uint32_vec_destroy(&((*b)->restarts));
		ubuf_destroy(&((*b)->buf));
		free((*b)->last_key);
		free((*b));
		*b = NULL;
	}
}

void
mtbl_block_builder_reset(struct mtbl_block_builder *b)
{
	ubuf_reset(b->buf);
	uint32_vec_reset(b->restarts);
	uint32_vec_add(b->restarts, 0);
	b->counter = 0;
	b->finished = false;
	free(b->last_key);
	b->last_key = NULL;
}

size_t
mtbl_block_builder_current_size_estimate(struct mtbl_block_builder *b)
{
	return (ubuf_bytes(b->buf) + uint32_vec_bytes(b->restarts) + sizeof(uint32_t));
}

void
mtbl_block_builder_finish(struct mtbl_block_builder *b, uint8_t **buf, size_t *bufsz)
{
	ubuf_need(b->buf, uint32_vec_bytes(b->restarts) + sizeof(uint32_t));

	for (size_t i = 0; i < uint32_vec_size(b->restarts); i++) {
		fprintf(stderr, "%s: writing restart value: %u\n", __func__,
			(unsigned) uint32_vec_value(b->restarts, i));
		mtbl_fixed_encode32(ubuf_ptr(b->buf), uint32_vec_value(b->restarts, i));
		ubuf_advance(b->buf, sizeof(uint32_t));
	}

	fprintf(stderr, "%s: writing number of restarts: %u\n", __func__,
		(unsigned) uint32_vec_size(b->restarts));
	mtbl_fixed_encode32(ubuf_ptr(b->buf), uint32_vec_size(b->restarts));
	ubuf_advance(b->buf, sizeof(uint32_t));

	b->finished = true;
	ubuf_detach(b->buf, buf, bufsz);
}

void
mtbl_block_builder_add(struct mtbl_block_builder *b,
		       const uint8_t *key, size_t len_key,
		       const uint8_t *val, size_t len_val)
{
	assert(b->counter <= b->block_restart_interval);
	assert(b->finished == false);

	fprintf(stderr, "============================\n%s: writing len_key= %zd, len_val= %zd\n",
		__func__, len_key, len_val);

	size_t shared = 0;

	/* see how much sharing to do with previous key */
	if (b->last_key != NULL && b->counter < b->block_restart_interval) {
		const size_t min_length = (b->len_last_key > len_key) ?
			(len_key) : (b->len_last_key);
		while ((shared < min_length) && (b->last_key[shared] == key[shared]))
			shared++;
	} else {
		/* restart compression */
		uint32_vec_add(b->restarts, (uint32_t) ubuf_bytes(b->buf));
		b->counter = 0;
	}
	const size_t non_shared = len_key - shared;

	/* ensure enough buffer space is available */
	ubuf_need(b->buf, 5*3 + non_shared + len_val);

	/* add "[shared][non-shared][value length]" to buffer */
	fprintf(stderr, "%s: writing value %u\n", __func__, (unsigned) shared);
	ubuf_advance(b->buf, mtbl_varint_encode32(ubuf_ptr(b->buf), shared));

	fprintf(stderr, "%s: writing value %u\n", __func__, (unsigned) non_shared);
	ubuf_advance(b->buf, mtbl_varint_encode32(ubuf_ptr(b->buf), non_shared));

	fprintf(stderr, "%s: writing value %u\n", __func__, (unsigned) len_val);
	ubuf_advance(b->buf, mtbl_varint_encode32(ubuf_ptr(b->buf), len_val));

	/* add key suffix to buffer followed by value */
	fprintf(stderr, "%s: writing %u bytes\n", __func__, (unsigned) non_shared);
	memcpy(ubuf_ptr(b->buf), key + shared, non_shared);
	ubuf_advance(b->buf, non_shared);

	fprintf(stderr, "%s: writing %u bytes\n", __func__, (unsigned) len_val);
	memcpy(ubuf_ptr(b->buf), val, len_val);
	ubuf_advance(b->buf, len_val);

	/* update state */
	free(b->last_key);
	b->len_last_key = len_key;
	b->last_key = malloc(len_key);
	assert(b->last_key != NULL);
	memcpy(b->last_key, key, len_key);
	b->counter += 1;
}
