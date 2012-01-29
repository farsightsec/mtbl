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

#define bytes_used(b) ((b)->p - (b)->buf)

static void	buffer_needs(struct mtbl_block_builder *b, size_t needed);

struct mtbl_block_builder {
	size_t		block_restart_interval;
	size_t		block_size;

	uint8_t		*p;
	uint8_t		*buf;
	size_t		bufsz;

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

	b->bufsz = b->block_size;
	b->buf = malloc(b->bufsz);
	if (b->buf == NULL) {
		free(b);
		return (NULL);
	}
	b->p = b->buf;

	b->restarts = uint32_vec_init(64);
	uint32_vec_add(b->restarts, 0);

	return (b);
}

void
mtbl_block_builder_destroy(struct mtbl_block_builder **b)
{
	if (*b) {
		uint32_vec_destroy(&((*b)->restarts));
		free((*b)->last_key);
		free((*b)->buf);
		free((*b));
		*b = NULL;
	}
}

void
mtbl_block_builder_reset(struct mtbl_block_builder *b)
{
	b->p = b->buf;
	b->bufsz = b->block_size;
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
	return (bytes_used(b) + uint32_vec_bytes(b->restarts) + sizeof(uint32_t));
}

void
mtbl_block_builder_finish(struct mtbl_block_builder *b, uint8_t **buf, size_t *bufsz)
{
	size_t needed = uint32_vec_bytes(b->restarts) + sizeof(uint32_t);

	buffer_needs(b, needed);

	for (size_t i = 0; i < b->restarts->n; i++) {
		fprintf(stderr, "%s:%d: writing restart value: %u\n", __func__, __LINE__,
			(unsigned) b->restarts->v[i]);
		mtbl_fixed_encode32(b->p, b->restarts->v[i]);
		b->p += sizeof(uint32_t);
	}

	fprintf(stderr, "%s:%d: writing number of restarts: %u\n", __func__, __LINE__,
		(unsigned) b->restarts->n);
	mtbl_fixed_encode32(b->p, b->restarts->n);
	b->p += sizeof(uint32_t);
	
	b->finished = true;
	*buf = b->buf;
	*bufsz = bytes_used(b);
}

void
mtbl_block_builder_add(struct mtbl_block_builder *b,
		       const uint8_t *key, size_t len_key,
		       const uint8_t *val, size_t len_val)
{
	assert(b->counter <= b->block_restart_interval);
	assert(b->finished == false);

	fprintf(stderr, "============================\n%s:%d: writing len_key= %zd, len_val= %zd\n",
		__func__, __LINE__, len_key, len_val);

	size_t shared = 0;

	/* see how much sharing to do with previous key */
	if (b->last_key != NULL && b->counter < b->block_restart_interval) {
		const size_t min_length = (b->len_last_key > len_key) ?
			(len_key) : (b->len_last_key);
		while ((shared < min_length) && (b->last_key[shared] == key[shared]))
			shared++;
	} else {
		/* restart compression */
		uint32_vec_add(b->restarts, (uint32_t) bytes_used(b));
		b->counter = 0;
	}
	const size_t non_shared = len_key - shared;

	/* ensure enough buffer space is available */
	buffer_needs(b, 5*3 + non_shared + len_val);

	/* add "[shared][non-shared][value length]" to buffer */
	fprintf(stderr, "%s:%d: writing value %u\n", __func__, __LINE__, (unsigned) shared);
	mtbl_varint_encode32(&b->p, shared);

	fprintf(stderr, "%s:%d: writing value %u\n", __func__, __LINE__, (unsigned) non_shared);
	mtbl_varint_encode32(&b->p, non_shared);

	fprintf(stderr, "%s:%d: writing value %u\n", __func__, __LINE__, (unsigned) len_val);
	mtbl_varint_encode32(&b->p, len_val);

	/* add key suffix to buffer followed by value */
	fprintf(stderr, "%s:%d: writing %u bytes\n", __func__, __LINE__, (unsigned) non_shared);
	memcpy(b->p, key + shared, non_shared);
	b->p += non_shared;

	fprintf(stderr, "%s:%d: writing %u bytes\n", __func__, __LINE__, (unsigned) len_val);
	memcpy(b->p, val, len_val);
	b->p += len_val;

	/* update state */
	free(b->last_key);
	b->len_last_key = len_key;
	b->last_key = malloc(len_key);
	assert(b->last_key != NULL);
	memcpy(b->last_key, key, len_key);
	b->counter += 1;
}

static void
buffer_needs(struct mtbl_block_builder *b, size_t needed)
{
	if (bytes_used(b) + needed >= b->bufsz) {
		b->bufsz += b->block_size;
		b->buf = realloc(b->buf, b->bufsz);
		assert(b->buf != NULL);
	}
}
